
#include "msgQLib.h"
#include "tickLib.h"

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct MsgNode {
    int prio;
    size_t len;
    unsigned char* data;
    struct MsgNode* next;
} MsgNode;

typedef struct MQ {
    pthread_mutex_t mtx;
    pthread_cond_t  canSend;
    pthread_cond_t  canRecv;
    size_t maxMsgs, maxLen;
    int priority;     // 0=fifo, 1=priority
    int valid;        // 1 while queue usable
    size_t count;
    MsgNode* head;
    MsgNode* tail;    // used for FIFO fast append
} MQ;

static int add_ms_to_timespec(struct timespec* ts, unsigned long long ms) {
    const long NS_PER_MS = 1000000L;
    if (!ts) return -1;
    ts->tv_nsec += (long)(ms % 1000ULL) * NS_PER_MS;
    ts->tv_sec  += (time_t)(ms / 1000ULL);
    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_nsec -= 1000000000L;
        ts->tv_sec  += 1;
    }
    return 0;
}

static int wait_pred_with_timeout(pthread_cond_t* cv, pthread_mutex_t* mtx,
                                  int timeoutTicks, int (*pred)(void*), void* ctx) {
    if (timeoutTicks == 0) {
        return pred(ctx); // 1 if ready, 0 if not
    } else if (timeoutTicks < 0) {
        while (!pred(ctx)) {
            pthread_cond_wait(cv, mtx);
        }
        return 1;
    } else {
        int tps = sysClkRateGet(); if (tps <= 0) tps = 60;
        unsigned long long ms = ((unsigned long long)timeoutTicks * 1000ULL) / (unsigned long long)tps;
        struct timespec deadline;
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        add_ms_to_timespec(&deadline, ms);
        while (!pred(ctx)) {
            int rc = pthread_cond_timedwait(cv, mtx, &deadline);
            if (rc == ETIMEDOUT) return pred(ctx) ? 1 : 0;
            if (rc != 0) return 0; // treat error as failure
        }
        return 1;
    }
}

static int pred_can_send(void* ctx) {
    MQ* q = (MQ*)ctx;
    return !q->valid ? 1 : (q->count < q->maxMsgs);
}

static int pred_has_data(void* ctx) {
    MQ* q = (MQ*)ctx;
    return !q->valid ? 1 : (q->count > 0);
}

MSG_Q_ID msgQCreate(size_t maxMsgs, size_t maxMsgLen, int options) {
    if (maxMsgs == 0 || maxMsgLen == 0) return NULL;
    MQ* q = (MQ*)calloc(1, sizeof(MQ));
    if (!q) return NULL;
    if (pthread_mutex_init(&q->mtx, NULL) != 0) { free(q); return NULL; }
    if (pthread_cond_init(&q->canSend, NULL) != 0) { pthread_mutex_destroy(&q->mtx); free(q); return NULL; }
    if (pthread_cond_init(&q->canRecv, NULL) != 0) {
        pthread_cond_destroy(&q->canSend);
        pthread_mutex_destroy(&q->mtx);
        free(q);
        return NULL;
    }
    q->maxMsgs = maxMsgs;
    q->maxLen = maxMsgLen;
    q->priority = (options & 1) ? 1 : 0;
    q->valid = 1;
    q->count = 0;
    q->head = q->tail = NULL;
    return q;
}

int msgQDelete(MSG_Q_ID id) {
    if (!id) return -1;
    MQ* q = (MQ*)id;
    pthread_mutex_lock(&q->mtx);
    q->valid = 0;
    pthread_cond_broadcast(&q->canSend);
    pthread_cond_broadcast(&q->canRecv);
    // free all nodes
    MsgNode* n = q->head;
    q->head = q->tail = NULL;
    q->count = 0;
    pthread_mutex_unlock(&q->mtx);

    while (n) {
        MsgNode* next = n->next;
        free(n->data);
        free(n);
        n = next;
    }

    pthread_cond_destroy(&q->canRecv);
    pthread_cond_destroy(&q->canSend);
    pthread_mutex_destroy(&q->mtx);
    free(q);
    return 0;
}

int msgQSend(MSG_Q_ID id, const void* buf, size_t nbytes, int timeoutTicks, int priority) {
    if (!id || (!buf && nbytes>0)) return -1;
    MQ* q = (MQ*)id;
    pthread_mutex_lock(&q->mtx);
    if (!q->valid) { pthread_mutex_unlock(&q->mtx); return -1; }

    if (!wait_pred_with_timeout(&q->canSend, &q->mtx, timeoutTicks, pred_can_send, q)) {
        pthread_mutex_unlock(&q->mtx);
        return -1;
    }
    if (!q->valid) { pthread_mutex_unlock(&q->mtx); return -1; }

    size_t len = nbytes > q->maxLen ? q->maxLen : nbytes;
    MsgNode* node = (MsgNode*)malloc(sizeof(MsgNode));
    if (!node) { pthread_mutex_unlock(&q->mtx); return -1; }
    node->prio = priority;
    node->len = len;
    node->data = (unsigned char*)malloc(len ? len : 1);
    if (!node->data) { free(node); pthread_mutex_unlock(&q->mtx); return -1; }
    if (len && buf) memcpy(node->data, buf, len);
    node->next = NULL;

    if (!q->priority) {
        // FIFO append
        if (!q->tail) q->head = q->tail = node;
        else { q->tail->next = node; q->tail = node; }
    } else {
        // Insert by priority (lower number = higher priority)
        MsgNode** cur = &q->head;
        MsgNode* prev = NULL;
        while (*cur && (*cur)->prio <= node->prio) {
            prev = *cur;
            cur = &((*cur)->next);
        }
        node->next = *cur;
        if (prev == NULL) q->head = node;
        else prev->next = node;
        if (node->next == NULL) q->tail = node; // inserted at end
    }
    q->count += 1;
    pthread_cond_signal(&q->canRecv);
    pthread_mutex_unlock(&q->mtx);
    return 0;
}

int msgQReceive(MSG_Q_ID id, void* buf, size_t maxNBytes, int timeoutTicks, size_t* outLen) {
    if (!id) return -1;
    MQ* q = (MQ*)id;
    pthread_mutex_lock(&q->mtx);
    if (!q->valid) { pthread_mutex_unlock(&q->mtx); return -1; }

    if (!wait_pred_with_timeout(&q->canRecv, &q->mtx, timeoutTicks, pred_has_data, q)) {
        pthread_mutex_unlock(&q->mtx);
        return -1;
    }
    if (!q->valid) { pthread_mutex_unlock(&q->mtx); return -1; }
    if (q->count == 0) { pthread_mutex_unlock(&q->mtx); return -1; }

    // pop from head
    MsgNode* node = q->head;
    q->head = node->next;
    if (!q->head) q->tail = NULL;
    q->count -= 1;

    size_t actual = node->len;
    size_t toCopy = (buf && maxNBytes>0) ? (actual < maxNBytes ? actual : maxNBytes) : 0;
    if (toCopy) memcpy(buf, node->data, toCopy);
    if (outLen) *outLen = actual;

    pthread_cond_signal(&q->canSend);
    pthread_mutex_unlock(&q->mtx);

    free(node->data);
    free(node);
    return 0;
}
