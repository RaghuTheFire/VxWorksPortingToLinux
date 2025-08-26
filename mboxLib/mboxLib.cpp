#include "mboxLib.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <thread>

// Assuming a typical real-time system with 1000 ticks per second
static constexpr int TICKS_PER_SEC = 1000;

/**
 * @struct Mbox
 * @brief Mailbox data structure for inter-task communication
 * 
 * This structure contains all the necessary components for a thread-safe
 * mailbox implementation suitable for real-time systems.
 */
struct Mbox {
    std::queue<void*> q;               ///< Message queue
    int capacity;                      ///< Maximum number of messages the mailbox can hold
    std::mutex mtx;                    ///< Mutex for protecting access to the mailbox
    std::condition_variable cvSend;    ///< Condition variable for send operations
    std::condition_variable cvRecv;    ///< Condition variable for receive operations
    std::atomic<bool> valid{true};     ///< Flag indicating if the mailbox is valid/active
};

/**
 * @brief Creates a new mailbox
 * 
 * @param capacity The maximum number of messages the mailbox can hold
 * @return MBOX_ID Handle to the created mailbox, or nullptr if creation failed
 * 
 * @note In a real-time system, consider using a custom allocator that avoids
 *       non-deterministic memory allocation behavior.
 */
MBOX_ID mboxCreate(int capacity) {
    if (capacity <= 0) return nullptr;
    
    try {
        auto mb = new Mbox();
        mb->capacity = capacity;
        return mb;
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

/**
 * @brief Deletes a mailbox and cleans up resources
 * 
 * @param mboxId Handle to the mailbox to delete
 * @return int 0 on success, -1 on failure
 * 
 * @note This function marks the mailbox as invalid and wakes up all threads
 *       waiting on it to prevent deadlocks during shutdown.
 */
int mboxDelete(MBOX_ID mboxId) {
    if (!mboxId) return -1;
    
    auto mb = static_cast<Mbox*>(mboxId);
    
    // Mark mailbox as invalid
    mb->valid.store(false, std::memory_order_release);
    
    // Notify all waiting threads
    mb->cvSend.notify_all();
    mb->cvRecv.notify_all();
    
    // Delete the mailbox
    delete mb;
    return 0;
}

/**
 * @brief Sends a message to a mailbox
 * 
 * @param mboxId Handle to the mailbox
 * @param msg Pointer to the message to send
 * @param timeoutTicks Timeout in system ticks (-1 for infinite wait)
 * @return int 0 on success, -1 on failure or timeout
 * 
 * @note For real-time systems, consider using priority inheritance mutexes
 *       if your platform supports them (e.g., pthread_mutex with PTHREAD_PRIO_INHERIT)
 */
int mboxSend(MBOX_ID mboxId, void* msg, int timeoutTicks) {
    if (!mboxId) return -1;
    
    auto mb = static_cast<Mbox*>(mboxId);
    std::unique_lock<std::mutex> lock(mb->mtx);
    
    // Calculate timeout if specified
    auto timeoutTime = (timeoutTicks >= 0) ? 
        std::chrono::steady_clock::now() + 
        std::chrono::microseconds((timeoutTicks * 1000000) / TICKS_PER_SEC) :
        std::chrono::steady_clock::time_point::max();
    
    // Wait for space in the mailbox or timeout
    bool waitSuccess = mb->cvSend.wait_until(lock, timeoutTime, [mb] {
        return !mb->valid.load(std::memory_order_acquire) || 
               mb->q.size() < static_cast<size_t>(mb->capacity);
    });
    
    // Check if mailbox is still valid and we didn't timeout
    if (!mb->valid.load(std::memory_order_acquire) || !waitSuccess) {
        return -1;
    }
    
    // Add message to queue
    mb->q.push(msg);
    
    // Notify one waiting receiver
    lock.unlock();
    mb->cvRecv.notify_one();
    
    return 0;
}

/**
 * @brief Receives a message from a mailbox
 * 
 * @param mboxId Handle to the mailbox
 * @param pMsg Pointer to where the received message will be stored
 * @param timeoutTicks Timeout in system ticks (-1 for infinite wait)
 * @return int 0 on success, -1 on failure or timeout
 * 
 * @note For real-time systems, ensure your condition variable implementation
 *       uses a monotonic clock to avoid issues with system time changes
 */
int mboxReceive(MBOX_ID mboxId, void** pMsg, int timeoutTicks) {
    if (!mboxId || !pMsg) return -1;
    
    auto mb = static_cast<Mbox*>(mboxId);
    std::unique_lock<std::mutex> lock(mb->mtx);
    
    // Calculate timeout if specified
    auto timeoutTime = (timeoutTicks >= 0) ? 
        std::chrono::steady_clock::now() + 
        std::chrono::microseconds((timeoutTicks * 1000000) / TICKS_PER_SEC) :
        std::chrono::steady_clock::time_point::max();
    
    // Wait for message in mailbox or timeout
    bool waitSuccess = mb->cvRecv.wait_until(lock, timeoutTime, [mb] {
        return !mb->valid.load(std::memory_order_acquire) || !mb->q.empty();
    });
    
    // Check if mailbox is still valid and we have a message
    if (!mb->valid.load(std::memory_order_acquire) || mb->q.empty()) {
        return -1;
    }
    
    // Retrieve message from queue
    *pMsg = mb->q.front();
    mb->q.pop();
    
    // Notify one waiting sender
    lock.unlock();
    mb->cvSend.notify_one();
    
    return 0;
}