# Watchdog Timer Library (C++)

This project provides a simple C++ watchdog timer library.  
It lets you monitor tasks, detect hangs, and take corrective action through callback functions.  
An example program is included to show how the library works.

---

## Files

- **`wdLib.h`** – Public header with the watchdog API (create, start, cancel, delete).  
- **`wdLib.cpp`** – Implementation of the watchdog using threads, mutexes, and condition variables.  
- **`Example.cpp`** – Example program demonstrating how to use the watchdog library.  

---

## Building

Use a modern C++ compiler (g++/clang++). For example:

```bash
g++ -std=c++11 Example.cpp wdLib.cpp -o watchdog_example -pthread
```

---

## Running

After building, run:

```bash
./watchdog_example
```

You’ll see the watchdog monitor a simulated critical task.  
Sometimes the task completes in time, sometimes it “hangs” and the watchdog expires.  
When that happens, the callback prints a message.

---

## Usage Example

```cpp
#include "wdLib.h"
#include <iostream>

void watchdogCallback(uintptr_t arg) {
    const char* taskName = reinterpret_cast<const char*>(arg);
    std::cout << "Watchdog expired for task: " << taskName << std::endl;
}

int main() {
    WDOG_ID wd = wdCreate();
    const char* name = "MyTask";

    wdStart(wd, 200, watchdogCallback, reinterpret_cast<uintptr_t>(name));

    // Simulate some work...
    // wdCancel(wd); // cancel if finished in time

    wdDelete(wd);
}
```

---

## API

- `WDOG_ID wdCreate()` – create a watchdog instance  
- `int wdDelete(WDOG_ID wdId)` – delete a watchdog (cancels if active)  
- `int wdStart(WDOG_ID wdId, int delayTicks, void (*func)(uintptr_t), uintptr_t arg)` – start a watchdog timer  
- `int wdCancel(WDOG_ID wdId)` – cancel an active watchdog  

---



---

## Sample Output

Here is an example of what you might see when running the program.  
The output varies because the task randomly hangs about 30% of the time.

```
Watchdog Library Example
=========================

--- Iteration 1 ---
Critical task started...
Working...
Task completed successfully

--- Iteration 2 ---
Critical task started...
Working...
Task is hanging (simulated)...
Watchdog expired for task: CriticalTask

--- Iteration 3 ---
Critical task started...
Working...
Task completed successfully

--- Iteration 4 ---
Critical task started...
Working...
Task completed successfully

--- Iteration 5 ---
Critical task started...
Working...
Task is hanging (simulated)...
Watchdog expired for task: CriticalTask

Example completed
```

## License

MIT License.  
