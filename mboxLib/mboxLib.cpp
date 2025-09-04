#include "mboxLib.h"
#include "tickLib.h"

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct MsgNode {
    size_t len;
    unsigned char* data;
    struct MsgNode* next;
} MsgNode;

typedef struct Mbox {
    pthread_mutex_t mtx;
    pthread_cond_t  canSend;
    pthread_cond_t  canRecv;
    pthread_cond_t  drain;     // signals waiter count drops
    size_t maxMsgs;
    size_t maxLen;
    int    valid;              // 1 while usable
    size_t count;              // # messages in queue
    size_t waiterSend;
    size_t waiterRecv;
    MsgNode* head;
    MsgNode* tail;
} Mbox;

static int add_ms_to_timespec(struct timespec* ts, unsigned long long ms) {
    const long NS_PER_MS = 1000000L;
    ts->tv_nsec += (long)(ms % 1000ULL) * NS_PER_MS;
    ts->tv_sec  += (time_t)(ms / 1000ULL);
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec  += 1;
    }
    return 0;
}

static int wait_pred_with_timeout(pthread_cond_t* cv, pthread_mutex_t* mtx,
                                  int timeoutTicks, int (*pred)(void*), void* ctx,
                                  size_t* waiterCounter, pthread_cond_t* drain) {
    if (waiterCounter) (*waiterCounter)++;
    if (drain) pthread_cond_broadcast(drain);

    int ready = 0;
    if (timeoutTicks == 0) {
        ready = pred(ctx);
    } else if (timeoutTicks < 0) {
        while (!(ready = pred(ctx))) {
            pthread_cond_wait(cv, mtx);
        }
    } else {
        int tps = sysClkRateGet(); if (tps <= 0) tps = 60;
        unsigned long long ms = ((unsigned long long)timeoutTicks * 1000ULL) / (unsigned long long)tps;
        struct timespec deadline;
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        add_ms_to_timespec(&deadline, ms);
        while (!(ready = pred(ctx))) {
            int rc = pthread_cond_timedwait(cv, mtx, &deadline);
            if (rc == ETIMEDOUT) { ready = pred(ctx); break; }
            if (rc != 0) { ready = 0; break; }
        }
    }

    if (waiterCounter) {
        (*waiterCounter)--;
        if (drain) pthread_cond_broadcast(drain);
    }
    return ready;
}

static int pred_can_send(void* ctx) {
    Mbox* m = (Mbox*)ctx;
    return !m->valid ? 1 : (m->count < m->maxMsgs);
}

static int pred_has_data(void* ctx) {
    Mbox* m = (Mbox*)ctx;
    return !m->valid ? 1 : (m->count > 0);
}

MBOX_ID mboxCreate(size_t maxMsgs, size_t maxMsgLen) {
    if (maxMsgs == 0 || maxMsgLen == 0) return NULL;
    Mbox* m = (Mbox*)calloc(1, sizeof(Mbox));
    if (!m) return NULL;
    pthread_mutex_init(&m->mtx, NULL);
    pthread_cond_init(&m->canSend, NULL);
    pthread_cond_init(&m->canRecv, NULL);
    pthread_cond_init(&m->drain, NULL);
    m->maxMsgs = maxMsgs;
    m->maxLen  = maxMsgLen;
    m->valid   = 1;
    m->count   = 0;
    return m;
}

int mboxDelete(MBOX_ID id) {
    if (!id) return -1;
    Mbox* m = (Mbox*)id;
    pthread_mutex_lock(&m->mtx);
    m->valid = 0;
    pthread_cond_broadcast(&m->canSend);
    pthread_cond_broadcast(&m->canRecv);

    while (m->waiterSend > 0 || m->waiterRecv > 0) {
        pthread_cond_wait(&m->drain, &m->mtx);
    }

    MsgNode* n = m->head;
    m->head = m->tail = NULL;
    m->count = 0;
    pthread_mutex_unlock(&m->mtx);

    while (n) {
        MsgNode* next = n->next;
        free(n->data);
        free(n);
        n = next;
    }

    pthread_cond_destroy(&m->drain);
    pthread_cond_destroy(&m->canRecv);
    pthread_cond_destroy(&m->canSend);
    pthread_mutex_destroy(&m->mtx);
    free(m);
    return 0;
}

int mboxSend(MBOX_ID id, const void* data, size_t len, int timeoutTicks) {
    if (!id || (!data && len>0)) return -1;
    Mbox* m = (Mbox*)id;
    pthread_mutex_lock(&m->mtx);
    if (!m->valid) { pthread_mutex_unlock(&m->mtx); return -1; }

    if (!wait_pred_with_timeout(&m->canSend, &m->mtx, timeoutTicks, pred_can_send, m, &m->waiterSend, &m->drain)) {
        pthread_mutex_unlock(&m->mtx);
        return -1;
    }
    if (!m->valid || m->count >= m->maxMsgs) { pthread_mutex_unlock(&m->mtx); return -1; }

    size_t copyLen = len > m->maxLen ? m->maxLen : len;
    MsgNode* node = (MsgNode*)malloc(sizeof(MsgNode));
    node->len = copyLen;
    node->data = (unsigned char*)malloc(copyLen ? copyLen : 1);
    if (copyLen && data) memcpy(node->data, data, copyLen);
    node->next = NULL;

    if (!m->tail) m->head = m->tail = node;
    else { m->tail->next = node; m->tail = node; }
    m->count++;

    pthread_cond_signal(&m->canRecv);
    pthread_mutex_unlock(&m->mtx);
    return 0;
}

int mboxReceive(MBOX_ID id, void* buf, size_t maxLen, size_t* outLen, int timeoutTicks) {
    if (!id) return -1;
    Mbox* m = (Mbox*)id;
    pthread_mutex_lock(&m->mtx);
    if (!m->valid) { pthread_mutex_unlock(&m->mtx); return -1; }

    if (!wait_pred_with_timeout(&m->canRecv, &m->mtx, timeoutTicks, pred_has_data, m, &m->waiterRecv, &m->drain)) {
        pthread_mutex_unlock(&m->mtx);
        return -1;
    }
    if (!m->valid || m->count == 0) { pthread_mutex_unlock(&m->mtx); return -1; }

    MsgNode* node = m->head;
    m->head = node->next;
    if (!m->head) m->tail = NULL;
    m->count--;

    size_t actual = node->len;
    size_t toCopy = (buf && maxLen>0) ? (actual < maxLen ? actual : maxLen) : 0;
    if (toCopy) memcpy(buf, node->data, toCopy);
    if (outLen) *outLen = actual;

    pthread_cond_signal(&m->canSend);
    pthread_mutex_unlock(&m->mtx);

    free(node->data);
    free(node);
    return 0;
}
