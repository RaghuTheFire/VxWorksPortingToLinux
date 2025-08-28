/**
 * @file tickLibDemo.cpp
 * @brief Demo program using tickLib (VxWorks-like).
 *
 * This demonstrates initialization, reading, and delay
 * using the VxWorks-style tickLib on Linux.
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "tickLib.h"

int main()
{
    std::cout << "Initializing tickLib..." << std::endl;
    if (tickLibInit() != 0)
    {
        std::cerr << "tickLibInit failed!" << std::endl;
        return -1;
    }

    std::cout << "System clock rate = " << sysClkRateGet() << " ticks/sec" << std::endl;

    std::cout << "Sleeping 2 seconds (using std::this_thread::sleep_for)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    UINT tick1 = tickGet();
    std::cout << "Tick after 2s = " << tick1 << std::endl;

    std::cout << "Sleeping another 3 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    UINT tick2 = tickGet();
    std::cout << "Tick after 5s total = " << tick2 << std::endl;

    std::cout << "Elapsed ticks = " << (tick2 - tick1) << std::endl;

    // Demo sysClkRateSet
    std::cout << "Changing system clock rate to 200 ticks/sec..." << std::endl;
    if (sysClkRateSet(200) == 0)
    {
        std::cout << "New clock rate = " << sysClkRateGet() << " ticks/sec" << std::endl;
    }
    else
    {
        std::cerr << "Failed to set sysClkRate!" << std::endl;
    }

    std::cout << "Sleeping 1 second and checking ticks again..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Tick now = " << tickGet() << std::endl;

    std::cout << "Demo finished." << std::endl;
    return 0;
}
