/**
 * @file mboxLib.h
 * @brief Mailbox Library API for Inter-Task Communication
 * 
 * This header defines a mailbox-based messaging system for inter-task communication
 * in embedded and real-time systems. The API provides thread-safe message passing
 * with configurable capacity and timeout support.
 * 
 * @note This implementation is designed for systems with a C++11 compatible compiler
 *       and standard library support.
 */

#ifndef MBOX_LIB_H
#define MBOX_LIB_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing a mailbox instance
 * 
 * This type is used to reference a mailbox created by mboxCreate().
 * The internal structure is hidden from the API user.
 */
typedef void* MBOX_ID;

/**
 * @brief Creates a new mailbox with specified capacity
 * 
 * @param[in] capacity Maximum number of messages the mailbox can hold
 * @return MBOX_ID Handle to the created mailbox, or nullptr if creation failed
 * 
 * @note If the system runs out of memory, this function will return nullptr.
 * @note The capacity must be a positive integer greater than zero.
 */
MBOX_ID mboxCreate(int capacity);

/**
 * @brief Deletes a mailbox and releases all associated resources
 * 
 * @param[in] mboxId Handle to the mailbox to delete
 * @return int 0 on success, -1 on failure (invalid handle)
 * 
 * @note After calling this function, any threads waiting on the mailbox will
 *       be unblocked and receive an error return code.
 * @note It is the application's responsibility to ensure no threads are
 *       using the mailbox when it is deleted.
 */
int     mboxDelete(MBOX_ID mboxId);

/**
 * @brief Sends a message to a mailbox with optional timeout
 * 
 * @param[in] mboxId Handle to the target mailbox
 * @param[in] msg Pointer to the message to send
 * @param[in] timeoutTicks Maximum time to wait for space in the mailbox
 *                         (in system ticks). Use -1 for infinite wait.
 * @return int 0 on success, -1 on failure or timeout
 * 
 * @note If the mailbox is full, this function will block until space is available
 *       or the specified timeout expires.
 * @note The message content is not copied; only the pointer is stored. The sender
 *       retains ownership of the message memory until it is received.
 */
int     mboxSend(MBOX_ID mboxId, void* msg, int timeoutTicks);

/**
 * @brief Receives a message from a mailbox with optional timeout
 * 
 * @param[in] mboxId Handle to the source mailbox
 * @param[out] pMsg Pointer to a variable that will receive the message pointer
 * @param[in] timeoutTicks Maximum time to wait for a message to arrive
 *                         (in system ticks). Use -1 for infinite wait.
 * @return int 0 on success, -1 on failure or timeout
 * 
 * @note If the mailbox is empty, this function will block until a message arrives
 *       or the specified timeout expires.
 * @note The received message pointer is stored in *pMsg. The receiver assumes
 *       ownership of the message memory and is responsible for freeing it if necessary.
 */
int     mboxReceive(MBOX_ID mboxId, void** pMsg, int timeoutTicks);

#ifdef __cplusplus
}
#endif

#endif // MBOX_LIB_H