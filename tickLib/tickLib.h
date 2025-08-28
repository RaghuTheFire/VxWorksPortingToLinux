/**
 * @file tickLib.h
 * @brief VxWorks-like system tick library interface.
 *
 * Provides tick-based timing using an internal tick counter.
 * This is a simplified emulation of VxWorks tickLib for POSIX/Linux.
 *
 * Two backends are possible:
 * - C++ std::chrono::steady_clock
 * - POSIX clock_gettime(CLOCK_MONOTONIC)
 *
 * The implementation in tickLib.cpp will select the backend.
 */

#ifndef __INCtickLibh
#define __INCtickLibh

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup tickLib Tick Library
 *  @brief Emulation of VxWorks system tick library.
 *  @{
 */

/**
 * @brief Initialize the tick library.
 *
 * Starts internal time base and configures tick rate.
 *
 * @param ticksPerSec Tick frequency (ticks per second).
 * @return 0 on success, -1 on error.
 */
int tickLibInit (int ticksPerSec);

/**
 * @brief Get the number of system ticks since initialization.
 *
 * Equivalent to VxWorks ::tickGet.
 *
 * @return Current tick count (monotonic).
 */
uint64_t tickGet (void);

/**
 * @brief Get the tick rate (ticks per second).
 *
 * Equivalent to VxWorks ::sysClkRateGet.
 *
 * @return Configured tick frequency.
 */
int sysClkRateGet (void);

/**
 * @brief Set the tick rate (ticks per second).
 *
 * Equivalent to VxWorks ::sysClkRateSet.
 *
 * @param ticksPerSec New tick frequency.
 * @return 0 on success, -1 on error.
 */
int sysClkRateSet (int ticksPerSec);

/**
 * @brief Convert ticks to milliseconds.
 *
 * @param ticks Number of ticks.
 * @return Milliseconds equivalent.
 */
uint64_t tickToMs (uint64_t ticks);

/**
 * @brief Convert milliseconds to ticks.
 *
 * @param ms Milliseconds.
 * @return Equivalent ticks.
 */
uint64_t msToTicks (uint64_t ms);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __INCtickLibh */
