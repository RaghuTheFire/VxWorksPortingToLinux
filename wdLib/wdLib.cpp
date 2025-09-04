
#include "wdLib.h"
#include "tickLib.h"

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct WdControl {
    pthread_mutex_t mtx;
    pthread_cond_t  cv;
    pthread_t       thread;
    int             threadRunning;  // 0/1
    int             active;         // 0/1
    int             canceled;       // 0/1
    unsigned long long generation;
    WDOG_HANDLER    handler;
    uintptr_t       arg;
    struct timespec deadline;       // absolute CLOCK_MONOTONIC deadline
} WdControl;

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

static void* wd_worker(void* arg) {
    WdControl* wd = (WdControl*)arg;
    pthread_mutex_lock(&wd->mtx);
    unsigned long long myGen = wd->generation;
    struct timespec deadline = wd->deadline;
    for (;;) {
        if (!wd->active || wd->canceled || wd->generation != myGen) {
            // cancelled or superseded
            pthread_mutex_unlock(&wd->mtx);
            return NULL;
        }
        int rc = pthread_cond_timedwait(&wd->cv, &wd->mtx, &deadline);
        if (rc == ETIMEDOUT) {
            // Time to fire if still valid
            if (wd->active && !wd->canceled && wd->generation == myGen) {
                WDOG_HANDLER fn = wd->handler;
                uintptr_t a = wd->arg;
                wd->active = 0;
                pthread_mutex_unlock(&wd->mtx);
                if (fn) fn(a);
                return NULL;
            } else {
                pthread_mutex_unlock(&wd->mtx);
                return NULL;
            }
        } else if (rc == 0) {
            // Woken up early; loop to re-check predicate
            continue;
        } else {
            // Error; treat as cancel
            pthread_mutex_unlock(&wd->mtx);
            return NULL;
        }
    }
}

WDOG_ID wdCreate(void) {
    WdControl* wd = (WdControl*)calloc(1, sizeof(WdControl));
    if (!wd) return NULL;
    if (pthread_mutex_init(&wd->mtx, NULL) != 0) { free(wd); return NULL; }
    if (pthread_cond_init(&wd->cv, NULL) != 0) { pthread_mutex_destroy(&wd->mtx); free(wd); return NULL; }
    wd->threadRunning = 0;
    wd->active = 0;
    wd->canceled = 0;
    wd->generation = 0;
    wd->handler = NULL;
    wd->arg = 0;
    wd->deadline.tv_sec = 0;
    wd->deadline.tv_nsec = 0;
    return wd;
}

int wdDelete(WDOG_ID id) {
    if (!id) return -1;
    WdControl* wd = (WdControl*)id;
    // cancel and join if needed
    pthread_mutex_lock(&wd->mtx);
    wd->canceled = 1;
    wd->active = 0;
    wd->generation++;
    pthread_cond_broadcast(&wd->cv);
    pthread_mutex_unlock(&wd->mtx);

    if (wd->threadRunning) {
        pthread_join(wd->thread, NULL);
        wd->threadRunning = 0;
    }
    pthread_cond_destroy(&wd->cv);
    pthread_mutex_destroy(&wd->mtx);
    free(wd);
    return 0;
}

int wdCancel(WDOG_ID id) {
    if (!id) return -1;
    WdControl* wd = (WdControl*)id;
    pthread_mutex_lock(&wd->mtx);
    if (!wd->active) {
        pthread_mutex_unlock(&wd->mtx);
        return 0;
    }
    wd->canceled = 1;
    wd->active = 0;
    wd->generation++;
    pthread_cond_broadcast(&wd->cv);
    pthread_mutex_unlock(&wd->mtx);

    if (wd->threadRunning) {
        pthread_join(wd->thread, NULL);
        wd->threadRunning = 0;
    }
    return 0;
}

int wdStart(WDOG_ID id, int delayTicks, WDOG_HANDLER func, uintptr_t arg) {
    if (!id || !func || delayTicks < 0) return -1;
    WdControl* wd = (WdControl*)id;

    // cancel any existing timer
    (void)wdCancel(id);

    int tps = sysClkRateGet();
    if (tps <= 0) tps = 60;
    unsigned long long ms = ((unsigned long long)delayTicks * 1000ULL) / (unsigned long long)tps;

    pthread_mutex_lock(&wd->mtx);
    wd->handler = func;
    wd->arg = arg;
    wd->canceled = 0;
    wd->active = 1;
    wd->generation++;

    // set absolute deadline
    clock_gettime(CLOCK_MONOTONIC, &wd->deadline);
    add_ms_to_timespec(&wd->deadline, ms);

    unsigned long long myGen = wd->generation;
    pthread_mutex_unlock(&wd->mtx);

    // launch worker
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // joinable by default
    int rc = pthread_create(&wd->thread, &attr, wd_worker, wd);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        // rollback state
        pthread_mutex_lock(&wd->mtx);
        wd->active = 0;
        wd->canceled = 1;
        wd->generation = myGen; // already incremented; leave it
        pthread_mutex_unlock(&wd->mtx);
        return -1;
    }
    wd->threadRunning = 1;
    return 0;
}
