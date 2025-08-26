/**
 * @file semLib.h
 * @brief Semaphore Library Header File
 * @defgroup semLib Semaphore Library
 * @{
 */

#ifndef SEMLIB_H
#define SEMLIB_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def SEM_TYPE_BINARY
 * @brief Binary semaphore type identifier
 */
#define SEM_TYPE_BINARY   1

/**
 * @def SEM_TYPE_COUNTING
 * @brief Counting semaphore type identifier
 */
#define SEM_TYPE_COUNTING 2

/**
 * @def SEM_TYPE_MUTEX
 * @brief Mutex semaphore type identifier
 */
#define SEM_TYPE_MUTEX    3

/**
 * @def SEM_Q_FIFO
 * @brief FIFO queueing policy for semaphore waiters
 */
#define SEM_Q_FIFO  0x00

/**
 * @def SEM_Q_PRIORITY
 * @brief Priority-based queueing policy for semaphore waiters
 * @note Not implemented in this version - provided for compatibility only
 */
#define SEM_Q_PRIORITY 0x01

/**
 * @def OK
 * @brief Operation completed successfully
 */
#define OK  0

/**
 * @def ERROR
 * @brief Operation failed
 */
#define ERROR -1

/**
 * @struct SEM_ID_STRUCT
 * @brief Internal structure representing a semaphore
 * 
 * This structure contains a type identifier and a union that holds
 * either a POSIX semaphore or a pthread mutex depending on the semaphore type.
 */
typedef struct {
    int type;                       /**< Type of semaphore (BINARY, COUNTING, MUTEX) */
    union {
        sem_t posixSem;             /**< POSIX semaphore for binary & counting semaphores */
        pthread_mutex_t mutex;      /**< pthread mutex for mutex semaphores */
    };
} SEM_ID_STRUCT;

/**
 * @typedef SEM_ID
 * @brief Opaque pointer to a semaphore structure
 */
typedef SEM_ID_STRUCT* SEM_ID;

// API Functions

/**
 * @brief Creates a binary semaphore
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @param initialState Initial state of semaphore (0 = unavailable, 1 = available)
 * @return SEM_ID on success, NULL on failure
 */
SEM_ID semBCreate(int options, int initialState);

/**
 * @brief Creates a counting semaphore
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @param initialCount Initial value of the semaphore counter
 * @return SEM_ID on success, NULL on failure
 */
SEM_ID semCCreate(int options, int initialCount);

/**
 * @brief Creates a mutex semaphore
 * @param options Creation options (SEM_Q_FIFO or SEM_Q_PRIORITY)
 * @return SEM_ID on success, NULL on failure
 */
SEM_ID semMCreate(int options);

/**
 * @brief Attempts to acquire a semaphore
 * @param sem Semaphore to acquire
 * @param ticks Timeout specification:
 *              - -1: wait forever (block indefinitely)
 *              - 0: non-blocking, return immediately
 *              - >0: wait for specified number of ticks (approx 10ms per tick)
 * @return OK on success, ERROR on failure or timeout
 */
int semTake(SEM_ID sem, int ticks);

/**
 * @brief Releases a semaphore
 * @param sem Semaphore to release
 * @return OK on success, ERROR on failure
 */
int semGive(SEM_ID sem);

/**
 * @brief Deletes a semaphore and frees its resources
 * @param sem Semaphore to delete
 * @return OK on success, ERROR on failure
 */
int semDelete(SEM_ID sem);

#ifdef __cplusplus
}
#endif

#endif // SEMLIB_H

/** @} */ // end of semLib group