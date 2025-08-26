/**
 * @file msgQLib.c
 * @brief VxWorks-style message queue implementation with priority support
 * @details This implementation provides a POSIX-compatible message queue API
 * that supports both FIFO and priority-based message ordering. It uses pthreads
 * for synchronization and can be used as a drop-in replacement for VxWorks
 * message queues in many cases.
 */

#define _POSIX_C_SOURCE 200809L
#include "msgQLib.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

/**
 * @brief Default tick provider function (weakly linked)
 * @return Number of ticks per second (default: 100)
 * @details This function can be overridden by the application to provide
 * a custom tick rate. If not provided, defaults to 100 ticks per second.
 */
__attribute__((weak)) int vxTicksPerSecondGet(void) { return 100; }

/**
 * @brief Convert VxWorks ticks to absolute timespec
 * @param[in] ticks Number of ticks to convert (<0 forever, 0 no wait, >0 relative)
 * @param[out] ts Pointer to timespec structure to populate with absolute time
 * @details Converts VxWorks-style tick timeout to an absolute time suitable
 * for pthread_cond_timedwait. Handles negative values (wait forever) and
 * zero values (no wait) appropriately.
 */
static void ticks_to_abs_timespec(int ticks, struct timespec* ts) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    if (ticks < 0) { *ts = now; return; }
    int tps = vxTicksPerSecondGet(); if (tps <= 0) tps = 100;
    long ms = (long)((ticks * 1000LL) / tps);

    ts->tv_sec  = now.tv_sec + (ms / 1000);
    long nsec   = now.tv_nsec + (ms % 1000) * 1000000L;
    ts->tv_sec += nsec / 1000000000L;
    ts->tv_nsec = nsec % 1000000000L;
}

/**
 * @brief Priority queue node structure
 * @details Each node contains a message with its priority level and length,
 * forming a linked list for messages of the same priority.
 */
typedef struct priority_node {
    void* data;                 /**< Pointer to message data */
    size_t length;              /**< Length of message data in bytes */
    int priority;               /**< Priority level (0-255) */
    struct priority_node* next; /**< Pointer to next node in queue */
} priority_node_t;

/**
 * @brief Message queue structure
 * @details Contains all state information for a message queue, including
 * synchronization primitives and message storage. The structure varies
 * depending on whether the queue is in FIFO or priority mode.
 */
struct vx_msgq {
    int      options;           /**< Queue options (FIFO or PRIORITY) */
    int      maxMsgs;           /**< Maximum number of messages */
    int      maxMsgLen;         /**< Maximum message length in bytes */
    int      count;             /**< Current message count */
    
    /* For priority queue */
    priority_node_t* priority_heads[256];  /**< Array of head pointers for each priority level */
    priority_node_t* priority_tails[256];  /**< Array of tail pointers for each priority level */
    
    /* For FIFO mode */
    unsigned char* storage;     /**< Storage buffer for FIFO mode */
    size_t   slotSize;          /**< Size of each slot in FIFO mode */
    int      head;              /**< Head index for FIFO mode */
    int      tail;              /**< Tail index for FIFO mode */

    pthread_mutex_t lock;       /**< Mutex for thread synchronization */
    pthread_cond_t  canSend;    /**< Condition variable for send operations */
    pthread_cond_t  canRecv;    /**< Condition variable for receive operations */
    
    /* Memory pool for priority nodes */
    priority_node_t* node_pool; /**< Pre-allocated pool of nodes */
    void* data_pool;            /**< Pre-allocated data storage */
    int* free_nodes;            /**< List of free node indices */
    int free_count;             /**< Count of free nodes */
};

/**
 * @brief Initialize priority queue structures
 * @param[in] q Pointer to message queue structure
 * @return None
 * @details Allocates and initializes memory for priority queue operation.
 * Creates a memory pool for nodes and data to avoid frequent allocation.
 */
static void init_priority_queue(struct vx_msgq* q) {
    for (int i = 0; i < 256; i++) {
        q->priority_heads[i] = NULL;
        q->priority_tails[i] = NULL;
    }
    
    /* Allocate memory pool for nodes and data */
    q->node_pool = (priority_node_t*)malloc(q->maxMsgs * sizeof(priority_node_t));
    q->data_pool = malloc(q->maxMsgs * q->maxMsgLen);
    q->free_nodes = (int*)malloc(q->maxMsgs * sizeof(int));
    
    /* Initialize free list */
    q->free_count = q->maxMsgs;
    for (int i = 0; i < q->maxMsgs; i++) {
        q->free_nodes[i] = i;
        q->node_pool[i].data = (char*)q->data_pool + i * q->maxMsgLen;
    }
}

/**
 * @brief Clean up priority queue structures
 * @param[in] q Pointer to message queue structure
 * @return None
 * @details Frees memory allocated for priority queue operation.
 * Called when deleting a priority queue.
 */
static void cleanup_priority_queue(struct vx_msgq* q) {
    if (q->node_pool) free(q->node_pool);
    if (q->data_pool) free(q->data_pool);
    if (q->free_nodes) free(q->free_nodes);
}

/**
 * @brief Get a free node from the pool
 * @param[in] q Pointer to message queue structure
 * @return Pointer to allocated node or NULL if no nodes available
 * @details Retrieves a node from the pre-allocated memory pool.
 * This avoids frequent memory allocation and improves performance.
 */
static priority_node_t* alloc_node(struct vx_msgq* q) {
    if (q->free_count <= 0) return NULL;
    return &q->node_pool[q->free_nodes[--q->free_count]];
}

/**
 * @brief Return a node to the pool
 * @param[in] q Pointer to message queue structure
 * @param[in] node Pointer to node to free
 * @return None
 * @details Returns a node to the memory pool for reuse.
 * This avoids frequent memory deallocation and improves performance.
 */
static void free_node(struct vx_msgq* q, priority_node_t* node) {
    /* Calculate index of the node */
    int index = node - q->node_pool;
    if (index >= 0 && index < q->maxMsgs) {
        q->free_nodes[q->free_count++] = index;
    }
}

/**
 * @brief Create a message queue
 * @param[in] maxMsgs Maximum number of messages in the queue
 * @param[in] maxMsgLen Maximum length of each message in bytes
 * @param[in] options Queue options (MSG_Q_FIFO or MSG_Q_PRIORITY)
 * @return Message queue ID or NULL on error
 * @details Creates a new message queue with the specified parameters.
 * The queue can operate in FIFO or priority mode based on the options.
 * Allocates all necessary memory upfront for deterministic behavior.
 */
MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLen, int options) {
    if (maxMsgs <= 0 || maxMsgLen <= 0) return NULL;

    struct vx_msgq* q = (struct vx_msgq*)calloc(1, sizeof(*q));
    if (!q) return NULL;

    q->options    = options;
    q->maxMsgs    = maxMsgs;
    q->maxMsgLen  = maxMsgLen;
    q->count      = 0;

    if (options & MSG_Q_PRIORITY) {
        /* Priority queue mode */
        init_priority_queue(q);
        if (!q->node_pool || !q->data_pool || !q->free_nodes) {
            cleanup_priority_queue(q);
            free(q);
            return NULL;
        }
    } else {
        /* FIFO mode */
        q->slotSize   = sizeof(uint32_t) + (size_t)maxMsgLen;
        q->storage    = (unsigned char*)malloc(q->slotSize * (size_t)maxMsgs);
        if (!q->storage) { 
            free(q); 
            return NULL; 
        }
        q->head = q->tail = 0;
    }

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        if (options & MSG_Q_PRIORITY) {
            cleanup_priority_queue(q);
        } else {
            free(q->storage);
        }
        free(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->canSend, NULL) != 0) {
        pthread_mutex_destroy(&q->lock);
        if (options & MSG_Q_PRIORITY) {
            cleanup_priority_queue(q);
        } else {
            free(q->storage);
        }
        free(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->canRecv, NULL) != 0) {
        pthread_cond_destroy(&q->canSend);
        pthread_mutex_destroy(&q->lock);
        if (options & MSG_Q_PRIORITY) {
            cleanup_priority_queue(q);
        } else {
            free(q->storage);
        }
        free(q);
        return NULL;
    }
    
    return q;
}

/**
 * @brief Delete a message queue
 * @param[in] q Message queue ID to delete
 * @return OK on success, ERROR on failure
 * @details Deletes the specified message queue and frees all associated resources.
 * Any threads blocked on this queue will be unblocked with an error.
 * This function is thread-safe and can be called from any thread.
 */
STATUS msgQDelete(MSG_Q_ID q) {
    if (!q) return ERROR;
    
    /* Lock the queue to prevent any further operations */
    pthread_mutex_lock(&q->lock);
    
    if (q->options & MSG_Q_PRIORITY) {
        cleanup_priority_queue(q);
    } else {
        free(q->storage);
    }
    
    /* Unlock before destroying (though no one should be using it after delete) */
    pthread_mutex_unlock(&q->lock);
    
    /* Destroy synchronization primitives */
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->canSend);
    pthread_cond_destroy(&q->canRecv);
    
    /* Free the queue structure itself */
    free(q);
    return OK;
}

/**
 * @brief Write into ring slot (FIFO mode)
 * @param[in] q Pointer to message queue structure
 * @param[in] idx Slot index to write to
 * @param[in] buf Pointer to message data
 * @param[in] len Length of message data in bytes
 * @return None
 * @details Stores a message in the specified slot with length prefix.
 * The message is stored with a 4-byte length header followed by the data.
 */
static void slot_write(struct vx_msgq* q, int idx, const void* buf, size_t len) {
    unsigned char* base = q->storage + (size_t)idx * q->slotSize;
    uint32_t ulen = (uint32_t)len;
    memcpy(base, &ulen, sizeof(uint32_t));
    if (len) memcpy(base + sizeof(uint32_t), buf, len);
}

/**
 * @brief Read from ring slot (FIFO mode)
 * @param[in] q Pointer to message queue structure
 * @param[in] idx Slot index to read from
 * @param[out] out Buffer to receive message data
 * @param[in] max Maximum bytes to copy to output buffer
 * @return Actual length of the message (may be larger than max)
 * @details Reads a message from the specified slot, copying up to max bytes.
 * Returns the actual message length, which may be larger than the provided buffer.
 */
static size_t slot_read(struct vx_msgq* q, int idx, void* out, size_t max) {
    unsigned char* base = q->storage + (size_t)idx * q->slotSize;
    uint32_t ulen = 0;
    memcpy(&ulen, base, sizeof(uint32_t));
    size_t copy = (ulen > max) ? max : ulen;
    if (copy) memcpy(out, base + sizeof(uint32_t), copy);
    return (size_t)ulen;
}

/**
 * @brief Send a message to the queue
 * @param[in] q Message queue ID
 * @param[in] buffer Pointer to message data
 * @param[in] nBytes Length of message data in bytes
 * @param[in] ticks Timeout in ticks (<0 forever, 0 no wait, >0 relative)
 * @param[in] priority Message priority (0-255, higher is more important)
 * @return OK on success, ERROR on failure
 * @details Sends a message to the queue with optional timeout and priority.
 * In priority mode, higher priority messages will be received before lower priority ones.
 * This function is thread-safe and can be called from multiple threads concurrently.
 */
STATUS msgQSend(MSG_Q_ID q, const void* buffer, size_t nBytes, int ticks, int priority) {
    if (!q || !buffer || nBytes > (size_t)q->maxMsgLen) return ERROR;
    if (priority < 0) priority = 0;
    if (priority > 255) priority = 255;

    pthread_mutex_lock(&q->lock);

    /* Handle different timeout scenarios */
    if (ticks < 0) {
        /* wait forever for space */
        while (q->count == q->maxMsgs)
            pthread_cond_wait(&q->canSend, &q->lock);
    } else if (ticks == 0) {
        /* no wait - return immediately if queue is full */
        if (q->count == q->maxMsgs) { 
            pthread_mutex_unlock(&q->lock); 
            return ERROR; 
        }
    } else {
        /* wait with timeout */
        struct timespec ts; 
        ticks_to_abs_timespec(ticks, &ts);
        while (q->count == q->maxMsgs) {
            int r = pthread_cond_timedwait(&q->canSend, &q->lock, &ts);
            if (r == ETIMEDOUT) { 
                pthread_mutex_unlock(&q->lock); 
                return ERROR; 
            }
        }
    }

    /* Add message to queue based on mode */
    if (q->options & MSG_Q_PRIORITY) {
        /* Priority queue mode */
        priority_node_t* node = alloc_node(q);
        if (!node) { 
            pthread_mutex_unlock(&q->lock); 
            return ERROR; 
        }
        
        memcpy(node->data, buffer, nBytes);
        node->length = nBytes;
        node->priority = priority;
        node->next = NULL;
        
        /* Insert into priority queue */
        if (q->priority_heads[priority] == NULL) {
            q->priority_heads[priority] = node;
            q->priority_tails[priority] = node;
        } else {
            q->priority_tails[priority]->next = node;
            q->priority_tails[priority] = node;
        }
    } else {
        /* FIFO mode */
        slot_write(q, q->tail, buffer, nBytes);
        q->tail = (q->tail + 1) % q->maxMsgs;
    }
    
    q->count++;

    /* Signal waiting receivers */
    pthread_cond_signal(&q->canRecv);
    pthread_mutex_unlock(&q->lock);
    return OK;
}

/**
 * @brief Receive a message from the queue
 * @param[in] q Message queue ID
 * @param[out] buffer Buffer to receive message data
 * @param[in] maxNBytes Maximum bytes to receive
 * @param[in] ticks Timeout in ticks (<0 forever, 0 no wait, >0 relative)
 * @return Number of bytes received or -1 on error
 * @details Receives a message from the queue with optional timeout.
 * In priority mode, receives the highest priority message available.
 * If the message is longer than maxNBytes, it is truncated.
 * This function is thread-safe and can be called from multiple threads concurrently.
 */
int msgQReceive(MSG_Q_ID q, void* buffer, size_t maxNBytes, int ticks) {
    if (!q || !buffer || maxNBytes == 0) return -1;

    pthread_mutex_lock(&q->lock);

    /* Handle different timeout scenarios */
    if (ticks < 0) {
        /* wait forever for a message */
        while (q->count == 0)
            pthread_cond_wait(&q->canRecv, &q->lock);
    } else if (ticks == 0) {
        /* no wait - return immediately if queue is empty */
        if (q->count == 0) { 
            pthread_mutex_unlock(&q->lock); 
            return -1; 
        }
    } else {
        /* wait with timeout */
        struct timespec ts; 
        ticks_to_abs_timespec(ticks, &ts);
        while (q->count == 0) {
            int r = pthread_cond_timedwait(&q->canRecv, &q->lock, &ts);
            if (r == ETIMEDOUT) { 
                pthread_mutex_unlock(&q->lock); 
                return -1; 
            }
        }
    }

    size_t actualLen = 0;
    
    if (q->options & MSG_Q_PRIORITY) {
        /* Priority queue mode - find the highest priority message */
        int priority = 255;
        while (priority >= 0 && q->priority_heads[priority] == NULL) {
            priority--;
        }
        
        if (priority >= 0) {
            priority_node_t* node = q->priority_heads[priority];
            actualLen = node->length;
            size_t copy = (actualLen > maxNBytes) ? maxNBytes : actualLen;
            if (copy) memcpy(buffer, node->data, copy);
            
            /* Remove from queue */
            q->priority_heads[priority] = node->next;
            if (q->priority_heads[priority] == NULL) {
                q->priority_tails[priority] = NULL;
            }
            
            free_node(q, node);
        }
    } else {
        /* FIFO mode */
        actualLen = slot_read(q, q->head, buffer, maxNBytes);
        q->head = (q->head + 1) % q->maxMsgs;
    }
    
    q->count--;

    /* Signal waiting senders */
    pthread_cond_signal(&q->canSend);
    pthread_mutex_unlock(&q->lock);

    return (int)((actualLen > maxNBytes) ? maxNBytes : actualLen);
}