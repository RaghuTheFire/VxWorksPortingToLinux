#ifndef WD_LIB_H
#define WD_LIB_H

/**
 * @file wdLib.h
 * @brief Watchdog Timer Library Interface
 * 
 * This header defines the interface for a watchdog timer library that allows
 * creating, starting, canceling, and deleting software watchdog timers.
 * The library is designed to be compatible with both C and C++ code.
 */

#include <functional>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef WDOG_ID
 * @brief Opaque handle for watchdog timer instances
 * 
 * This type represents a handle to a watchdog timer instance. The internal
 * implementation details are hidden from the user.
 */
typedef void* WDOG_ID;

/**
 * @brief Creates a new watchdog timer instance
 * @return Handle to the newly created watchdog timer (WDOG_ID)
 * 
 * @note The caller is responsible for deleting the watchdog with wdDelete()
 */
WDOG_ID wdCreate();

/**
 * @brief Deletes a watchdog timer instance
 * @param wdId Handle to the watchdog timer to delete
 * @return 0 on success, -1 on error (invalid handle)
 * 
 * @note This function will cancel any active timer before deletion
 */
int     wdDelete(WDOG_ID wdId);

/**
 * @brief Starts a watchdog timer
 * @param wdId Handle to the watchdog timer
 * @param delayTicks Number of timer ticks before expiration
 * @param func Callback function to execute on expiration
 * @param arg Argument to pass to the callback function
 * @return 0 on success, -1 on error (invalid parameters)
 * 
 * @note If the timer is already active, it will be canceled first
 * @note The timer resolution is defined by TICKS_PER_SEC in the implementation
 */
int     wdStart(WDOG_ID wdId, int delayTicks, void (*func)(uintptr_t), uintptr_t arg);

/**
 * @brief Cancels an active watchdog timer
 * @param wdId Handle to the watchdog timer to cancel
 * @return 0 on success, -1 on error (invalid handle)
 * 
 * @note This function prevents the timer callback from being executed
 * @note If the timer has already expired, this function has no effect
 */
int     wdCancel(WDOG_ID wdId);

#ifdef __cplusplus
}
#endif

#endif // WD_LIB_H