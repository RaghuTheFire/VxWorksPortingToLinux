# Compilation Steps

## Prerequisites

-   **C++11 or later**
-   **POSIX Threads (pthreads) library**

Ensure you have `g++` (GNU C++ compiler) installed. On Ubuntu/Debian:

``` bash
sudo apt-get update
sudo apt-get install build-essential
```

------------------------------------------------------------------------

## Compiling the Library and Demo

1.  Open a terminal in the project root directory (where `msgQLib.cpp`,
    `msgQLib.h`, and `msgQLibDemo.cpp` are located).

2.  Run the following command to compile:

``` bash
g++ -o msgQDemo msgQLib.cpp msgQLibDemo.cpp -lpthread
```

-   `-o msgQDemo` : names the output executable as `msgQDemo`
-   `msgQLib.cpp` : library implementation
-   `msgQLibDemo.cpp` : demo application
-   `-lpthread` : links against the POSIX threads library

------------------------------------------------------------------------

## Running the Demo

After successful compilation, run the demo with:

``` bash
./msgQDemo
```

You should see console output for: 1. **FIFO Queue Demo** 2. **Priority
Queue Demo** 3. **Timeout Behavior Demo**

------------------------------------------------------------------------

## Optional: Clean Build

If you want to recompile from scratch, remove the executable first:

``` bash
rm msgQDemo
g++ -o msgQDemo msgQLib.cpp msgQLibDemo.cpp -lpthread
```

------------------------------------------------------------------------

## Next Steps

-   Integrate the library into your own project by including `msgQLib.h`
    and linking `msgQLib.cpp`.
-   Optionally, set up a **CMakeLists.txt** for easier builds across
    environments.
