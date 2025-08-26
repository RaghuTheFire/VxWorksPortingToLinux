/**
 * @file msgQLib.h
 * @brief VxWorks-style message queue library header
 * @details This header defines the API for a VxWorks-compatible message queue
 * implementation that supports both FIFO and priority-based message ordering.
 */

#ifndef _MSGQLIB_H
#define _MSGQLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status codes for message queue operations
 */
typedef int STATUS;
#define OK     0     /**< Operation successful */
#define ERROR -1     /**< Operation failed */

/**
 * @brief Opaque handle for message queue
 * @details The actual implementation is hidden from the user to maintain
 * abstraction and allow for future changes without breaking compatibility.
 */
typedef struct vx_msgq* MSG_Q_ID;

/**
 * @brief Message queue options
 */
#define MSG_Q_FIFO     0x00   /**< First-in-first-out message ordering */
#define MSG_Q_PRIORITY 0x01   /**< Priority-based message ordering */

/**
 * @brief Create a message queue
 * @param[in] maxMsgs Maximum number of messages in queue
 * @param[in] maxMsgLen Maximum length of each message in bytes
 * @param[in] options Queue options (MSG_Q_FIFO or MSG_Q_PRIORITY)
 * @return Message queue ID or NULL on error
 * @details Creates a new message queue with the specified parameters.
 * The queue can operate in FIFO or priority mode based on the options.
 */
MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLen, int options);

/**
 * @brief Delete a message queue
 * @param[in] q Message queue ID to delete
 * @return OK on success, ERROR on failure
 * @details Deletes the specified message queue and frees all associated resources.
 * Any threads blocked on this queue will be unblocked with an error.
 */
STATUS   msgQDelete(MSG_Q_ID q);

/**
 * @brief Send a message to a queue
 * @param[in] q Message queue ID
 * @param[in] buffer Pointer to message data
 * @param[in] nBytes Length of message data in bytes
 * @param[in] ticks Timeout in ticks (<0 forever, 0 no wait, >0 relative)
 * @param[in] priority Message priority (0-255, higher is more important)
 * @return OK on success, ERROR on failure
 * @details Sends a message to the queue with optional timeout and priority.
 * In priority mode, higher priority messages will be received before lower priority ones.
 */
STATUS msgQSend(MSG_Q_ID q, const void* buffer, size_t nBytes, int ticks, int priority);

/**
 * @brief Receive a message from a queue
 * @param[in] q Message queue ID
 * @param[out] buffer Buffer to receive message data
 * @param[in] maxNBytes Maximum bytes to receive
 * @param[in] ticks Timeout in ticks (<0 forever, 0 no wait, >0 relative)
 * @return Number of bytes received or -1 on error
 * @details Receives a message from the queue with optional timeout.
 * In priority mode, receives the highest priority message available.
 * If the message is longer than maxNBytes, it is truncated.
 */
int    msgQReceive(MSG_Q_ID q, void* buffer, size_t maxNBytes, int ticks);

/**
 * @brief Get system ticks per second
 * @return Number of ticks per second
 * @details This function is weakly linked and can be overridden by the application.
 * If not provided by the application, it defaults to 100 ticks per second.
 */
int vxTicksPerSecondGet(void);

#ifdef __cplusplus
}
#endif
#endif /* _MSGQLIB_H */