/**
 * @file tickLib.cpp
 * @brief VxWorks-like tick library for Linux (C++ implementation).
 *
 * Provides system tick counting functionality similar to VxWorks `tickLib`.
 * The tick counter is based on either:
 *   - std::chrono::steady_clock (default), or
 *   - POSIX clock_gettime(CLOCK_MONOTONIC)
 *
 * @note This library is designed for portability and real-time OS emulation.
 */

#include "tickLib.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef USE_POSIX_CLOCK
#include <time.h>
#endif

/* ---------------------------------------------------------------------- */
/* Internal state */
/* ---------------------------------------------------------------------- */

/// Global tick counter
static std::atomic<uint64_t> g_tickCount{0};

/// Tick rate (ticks per second)
static std::atomic<uint32_t> g_ticksPerSecond{60};  ///< Default 60 Hz

/// Tick thread control
static std::thread g_tickThread;
static std::atomic<bool> g_tickRunning{false};
static std::mutex g_tickMutex;
static std::condition_variable g_tickCv;

/* ---------------------------------------------------------------------- */
/* Internal helper: sleep per tick */
/* ---------------------------------------------------------------------- */

static void tickSleepLoop()
{
    using namespace std::chrono;

    uint32_t tps = g_ticksPerSecond.load();
    auto tickDuration = duration<double>(1.0 / static_cast<double>(tps));

    while (g_tickRunning.load())
    {
        {
            std::unique_lock<std::mutex> lock(g_tickMutex);
            g_tickCv.wait_for(lock, duration_cast<milliseconds>(tickDuration),
                              [] { return !g_tickRunning.load(); });
        }

        if (!g_tickRunning.load())
            break;

        tickAnnounce();
    }
}

/* ---------------------------------------------------------------------- */
/* Public API Implementation */
/* ---------------------------------------------------------------------- */

extern "C" {

/**
 * @brief Initialize tick library.
 */
STATUS tickLibInit(uint32_t ticksPerSecond)
{
    if (ticksPerSecond == 0)
        return ERROR;

    g_ticksPerSecond.store(ticksPerSecond);

    if (!g_tickRunning.load())
    {
        g_tickRunning.store(true);
        g_tickThread = std::thread(tickSleepLoop);
    }

    return OK;
}

/**
 * @brief Shutdown tick library.
 */
STATUS tickLibShutdown()
{
    if (g_tickRunning.load())
    {
        g_tickRunning.store(false);
        g_tickCv.notify_all();
        if (g_tickThread.joinable())
            g_tickThread.join();
    }
    return OK;
}

/**
 * @brief Announce one tick (increments tick counter).
 */
void tickAnnounce(void)
{
    g_tickCount.fetch_add(1);
}

/**
 * @brief Get current tick count.
 */
UINT32 tickGet(void)
{
    return static_cast<UINT32>(g_tickCount.load());
}

/**
 * @brief Set tick counter.
 */
void tickSet(UINT32 newTick)
{
    g_tickCount.store(newTick);
}

/**
 * @brief Get ticks per second.
 */
UINT32 sysClkRateGet(void)
{
    return g_ticksPerSecond.load();
}

/**
 * @brief Set ticks per second (changes tick thread rate).
 */
STATUS sysClkRateSet(UINT32 ticksPerSecond)
{
    if (ticksPerSecond == 0)
        return ERROR;

    g_ticksPerSecond.store(ticksPerSecond);
    g_tickCv.notify_all();  // wake tick thread to recompute rate
    return OK;
}

/**
 * @brief Get elapsed time (in ticks) since system start.
 */
UINT32 tickSinceBoot(void)
{
#ifdef USE_POSIX_CLOCK
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
    uint64_t ticks = ns / (1000000000ULL / g_ticksPerSecond.load());
    return static_cast<UINT32>(ticks);
#else
    using namespace std::chrono;
    static steady_clock::time_point start = steady_clock::now();
    auto elapsed = steady_clock::now() - start;
    uint64_t ticks = duration_cast<nanoseconds>(elapsed).count()
                     / (1000000000ULL / g_ticksPerSecond.load());
    return static_cast<UINT32>(ticks);
#endif
}

} // extern "C"
