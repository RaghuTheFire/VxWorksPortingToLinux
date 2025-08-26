/**
 * @file semLib.c
 * @brief Semaphore Library Implementation
 * @ingroup semLib
 * @{
 */

#include "semLib.h"
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Creates a binary semaphore
 * 
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @param initialState Initial state of the semaphore (0 = unavailable, 1 = available)
 * @return SEM_ID Pointer to the created semaphore, or NULL on failure
 * 
 * @note Binary semaphores have only two states: available (1) or unavailable (0)
 */
SEM_ID semBCreate(int options, int initialState) {
    SEM_ID s = (SEM_ID)malloc(sizeof(SEM_ID_STRUCT));
    if (!s) return NULL;

    s->type = SEM_TYPE_BINARY;
    sem_init(&s->posixSem, 0, initialState ? 1 : 0);
    return s;
}

/**
 * @brief Creates a counting semaphore
 * 
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @param initialCount Initial value of the semaphore counter
 * @return SEM_ID Pointer to the created semaphore, or NULL on failure
 * 
 * @note Counting semaphores can have any non-negative integer value
 */
SEM_ID semCCreate(int options, int initialCount) {
    SEM_ID s = (SEM_ID)malloc(sizeof(SEM_ID_STRUCT));
    if (!s) return NULL;

    s->type = SEM_TYPE_COUNTING;
    sem_init(&s->posixSem, 0, initialCount);
    return s;
}

/**
 * @brief Creates a mutex semaphore
 * 
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @return SEM_ID Pointer to the created mutex, or NULL on failure
 * 
 * @note Mutex semaphores are used for mutual exclusion between threads
 */
SEM_ID semMCreate(int options) {
    SEM_ID s = (SEM_ID)malloc(sizeof(SEM_ID_STRUCT));
    if (!s) return NULL;

    s->type = SEM_TYPE_MUTEX;
    pthread_mutex_init(&s->mutex, NULL);
    return s;
}

/**
 * @brief Attempts to acquire a semaphore
 * 
 * @param sem Semaphore to acquire
 * @param ticks Timeout specification:
 *              - -1: wait forever (block indefinitely)
 *              - 0: non-blocking, return immediately
 *              - >0: wait for specified number of ticks (approx 10ms per tick)
 * @return int OK on success, ERROR on failure or timeout
 * 
 * @note For mutex semaphores, this function provides recursive acquisition protection
 */
int semTake(SEM_ID sem, int ticks) {
    if (!sem) return ERROR;

    if (sem->type == SEM_TYPE_MUTEX) {
        if (ticks == -1) {
            return pthread_mutex_lock(&sem->mutex) == 0 ? OK : ERROR;
        } else if (ticks == 0) {
            return pthread_mutex_trylock(&sem->mutex) == 0 ? OK : ERROR;
        } else {
            // Convert ticks to absolute time for timed wait
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) return ERROR;
            
            // Assuming 100 ticks per second (10ms per tick)
            ts.tv_sec += ticks / 100;
            ts.tv_nsec += (ticks % 100) * 10000000; // 10ms in nanoseconds
            
            // Normalize nanoseconds to seconds if overflow
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            
            return pthread_mutex_timedlock(&sem->mutex, &ts) == 0 ? OK : ERROR;
        }
    } else {
        if (ticks == -1) {
            return sem_wait(&sem->posixSem) == 0 ? OK : ERROR;
        } else if (ticks == 0) {
            return sem_trywait(&sem->posixSem) == 0 ? OK : ERROR;
        } else {
            // Convert ticks to absolute time for timed wait
            struct timespec ts;
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) return ERROR;
            
            // Assuming 100 ticks per second (10ms per tick)
            ts.tv_sec += ticks / 100;
            ts.tv_nsec += (ticks % 100) * 10000000; // 10ms in nanoseconds
            
            // Normalize nanoseconds to seconds if overflow
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            
            return sem_timedwait(&sem->posixSem, &ts) == 0 ? OK : ERROR;
        }
    }
}

/**
 * @brief Releases a semaphore
 * 
 * @param sem Semaphore to release
 * @return int OK on success, ERROR on failure
 * 
 * @note For binary/counting semaphores, this increments the semaphore value.
 *       For mutex semaphores, this releases the lock.
 */
int semGive(SEM_ID sem) {
    if (!sem) return ERROR;

    if (sem->type == SEM_TYPE_MUTEX) {
        return pthread_mutex_unlock(&sem->mutex) == 0 ? OK : ERROR;
    } else {
        return sem_post(&sem->posixSem) == 0 ? OK : ERROR;
    }
}

/**
 * @brief Deletes a semaphore and frees its resources
 * 
 * @param sem Semaphore to delete
 * @return int OK on success, ERROR on failure
 * 
 * @warning It is the caller's responsibility to ensure no threads are waiting on
 *          the semaphore when it is deleted.
 */
int semDelete(SEM_ID sem) {
    if (!sem) return ERROR;

    if (sem->type == SEM_TYPE_MUTEX) {
        pthread_mutex_destroy(&sem->mutex);
    } else {
        sem_destroy(&sem->posixSem);
    }
    free(sem);
    return OK;
}

/** @} */ // end of semLib group