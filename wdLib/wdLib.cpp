#include "wdLib.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>

/**
 * @struct WdControl
 * @brief Internal watchdog control structure
 * 
 * This structure manages the internal state of a watchdog timer,
 * including thread synchronization and cancellation mechanisms.
 */
struct WdControl 
{
    std::thread th;                     ///< Thread running the watchdog timer
    std::mutex mtx;                     ///< Mutex for synchronization
    std::condition_variable cv;         ///< Condition variable for cancellation
    std::atomic<bool> active{false};    ///< Flag indicating if watchdog is active
    std::atomic<bool> canceled{false};  ///< Flag indicating if watchdog was canceled
};

static const int TICKS_PER_SEC = 100;   ///< Timer resolution in ticks per second

/**
 * @brief Creates a new watchdog timer instance
 * @return Handle to the newly created watchdog timer
 * 
 * @note The caller is responsible for deleting the watchdog with wdDelete()
 */
WDOG_ID wdCreate() 
{
    return new WdControl();
}

/**
 * @brief Deletes a watchdog timer instance
 * @param wdId Handle to the watchdog timer to delete
 * @return 0 on success, -1 on error
 * 
 * @note This function will cancel any active timer before deletion
 */
int wdDelete(WDOG_ID wdId) 
{
    if (!wdId) return -1;
    auto* wd = static_cast<WdControl*>(wdId);
    
    std::lock_guard<std::mutex> lock(wd->mtx);
    wd->active = false;
    wd->canceled = true;
    wd->cv.notify_all();
    
    if (wd->th.joinable()) {
        wd->th.join();
    }
    
    delete wd;
    return 0;
}

/**
 * @brief Starts a watchdog timer
 * @param wdId Handle to the watchdog timer
 * @param delayTicks Number of timer ticks before expiration
 * @param func Callback function to execute on expiration
 * @param arg Argument to pass to the callback function
 * @return 0 on success, -1 on error
 * 
 * @note If the timer is already active, it will be canceled first
 * @note The timer will be automatically canceled if wdCancel() is called
 */
int wdStart(WDOG_ID wdId, int delayTicks, void (*func)(uintptr_t), uintptr_t arg) 
{
    if (!wdId || !func || delayTicks <= 0) return -1;
    auto* wd = static_cast<WdControl*>(wdId);

    // Cancel any existing watchdog
    wdCancel(wdId);

    wd->active = true;
    wd->canceled = false;
    
    wd->th = std::thread([wd, delayTicks, func, arg]() {
        std::unique_lock<std::mutex> lock(wd->mtx);
        auto delay = std::chrono::milliseconds((delayTicks * 1000) / TICKS_PER_SEC);
        
        // Wait for either timeout or cancellation
        if (wd->cv.wait_for(lock, delay, [wd] { return wd->canceled.load(); })) {
            // Cancelled
            return;
        }
        
        // Only execute if still active
        if (wd->active.load()) 
        {
            lock.unlock();  // Unlock before callback to avoid potential deadlocks
            func(arg);
        }
    });

    return 0;
}

/**
 * @brief Cancels an active watchdog timer
 * @param wdId Handle to the watchdog timer to cancel
 * @return 0 on success, -1 on error
 * 
 * @note This function is thread-safe and can be called multiple times
 */
int wdCancel(WDOG_ID wdId) 
{
    if (!wdId) return -1;
    auto* wd = static_cast<WdControl*>(wdId);
    
    std::lock_guard<std::mutex> lock(wd->mtx);
    wd->canceled = true;
    wd->cv.notify_all();
    
    if (wd->th.joinable()) 
    {
        wd->th.join();
    }
    
    wd->active = false;
    wd->canceled = false;
    return 0;
}