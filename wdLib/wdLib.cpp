#include "wdLib.h"
#include "tickLib.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

namespace {
struct WdControl {
    std::mutex m;
    std::condition_variable cv;
    std::thread th;
    bool active{false};
    bool canceled{false};
    uint64_t generation{0};
    WDOG_HANDLER handler{nullptr};
    uintptr_t arg{0};
};
}

static void worker(WdControl* wd, uint64_t myGen, int delayTicks) {
    using namespace std::chrono;
    int tps = sysClkRateGet(); if (tps <= 0) tps = 60;
    uint64_t ms = (static_cast<uint64_t>(delayTicks) * 1000ull) / static_cast<uint64_t>(tps);
    auto deadline = steady_clock::now() + milliseconds(ms);
    std::unique_lock<std::mutex> lock(wd->m);
    wd->cv.wait_until(lock, deadline, [&]{
        return wd->canceled || !wd->active || wd->generation != myGen;
    });
    bool shouldFire = wd->active && !wd->canceled && wd->generation == myGen && steady_clock::now() >= deadline;
    WDOG_HANDLER fn = wd->handler;
    uintptr_t a = wd->arg;
    wd->active = false; // one-shot
    lock.unlock();
    if (shouldFire && fn) {
        fn(a);
    }
}

WDOG_ID wdCreate(void) {
    try { return new WdControl(); } catch (...) { return nullptr; }
}

int wdDelete(WDOG_ID id) {
    if (!id) return -1;
    WdControl* wd = static_cast<WdControl*>(id);
    // cancel and join if needed
    {
        std::lock_guard<std::mutex> lock(wd->m);
        wd->canceled = true;
        wd->active = false;
        wd->generation++;
        wd->cv.notify_all();
    }
    if (wd->th.joinable()) wd->th.join();
    delete wd;
    return 0;
}

int wdCancel(WDOG_ID id) {
    if (!id) return -1;
    WdControl* wd = static_cast<WdControl*>(id);
    {
        std::lock_guard<std::mutex> lock(wd->m);
        if (!wd->active) return 0;
        wd->canceled = true;
        wd->active = false;
        wd->generation++;
        wd->cv.notify_all();
    }
    if (wd->th.joinable()) wd->th.join();
    return 0;
}

int wdStart(WDOG_ID id, int delayTicks, WDOG_HANDLER func, uintptr_t arg) {
    if (!id || !func || delayTicks < 0) return -1;
    WdControl* wd = static_cast<WdControl*>(id);
    // cancel any existing timer
    wdCancel(id);
    {
        std::lock_guard<std::mutex> lock(wd->m);
        wd->handler = func;
        wd->arg = arg;
        wd->canceled = false;
        wd->active = true;
        wd->generation++;
        uint64_t gen = wd->generation;
        // launch worker
        wd->th = std::thread(worker, wd, gen, delayTicks);
    }
    return 0;
}
