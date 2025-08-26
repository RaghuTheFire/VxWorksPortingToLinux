#include <iostream>
#include <thread>
#include <chrono>
#include "wdLib.h"

// Example callback function for watchdog expiration
void watchdogCallback(uintptr_t arg) 
{
    const char* taskName = reinterpret_cast<const char*>(arg);
    std::cout << "Watchdog expired for task: " << taskName << std::endl;
    // In a real application, you might reset the system or take corrective action
}

// Simulated task that might hang
void criticalTask(WDOG_ID watchdogId) 
{
    std::cout << "Critical task started..." << std::endl;
    
    // Start watchdog with 2-second timeout (200 ticks at 100 ticks/sec)
    const char* taskName = "CriticalTask";
    if (wdStart(watchdogId, 200, watchdogCallback, reinterpret_cast<uintptr_t>(taskName)) 
	{
        std::cerr << "Failed to start watchdog" << std::endl;
        return;
    }
    
    // Simulate work - sometimes takes too long
    std::cout << "Working..." << std::endl;
    
    // 30% chance to simulate a hang
    if (rand() % 100 < 30) 
	{
        std::cout << "Task is hanging (simulated)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Exceeds watchdog timeout
    } 
	else 
	{
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Within watchdog timeout
        std::cout << "Task completed successfully" << std::endl;
        wdCancel(watchdogId); // Cancel watchdog if task completes in time
    }
}

int main() 
{
    std::cout << "Watchdog Library Example" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Create a watchdog
    WDOG_ID watchdog = wdCreate();
    if (!watchdog) 
	{
        std::cerr << "Failed to create watchdog" << std::endl;
        return 1;
    }
    
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Run the critical task multiple times to demonstrate both scenarios
    for (int i = 0; i < 5; i++) 
	{
        std::cout << "\n--- Iteration " << i+1 << " ---" << std::endl;
        criticalTask(watchdog);
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Brief pause between iterations
    }
    
    // Clean up
    wdDelete(watchdog);
    
    std::cout << "\nExample completed" << std::endl;
    return 0;
}