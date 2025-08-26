# Compilation Steps

## 1. File Overview

-   **`semLib.h`** -- header file with definitions and function
    prototypes.\
-   **`semLib.cpp`** -- implementation of the semaphore library.\
-   **`semLibDemo.cpp`** -- demonstration program using the library.

------------------------------------------------------------------------

## 2. Required Libraries

The code uses: - **POSIX threads (`pthread`)** - **POSIX semaphores
(`semaphore.h`)** - **C standard libraries (`time.h`, `stdlib.h`,
`stdio.h`)**

So you'll need to link against **`pthread`** when compiling.

------------------------------------------------------------------------

## 3. Compilation Steps

### Single Command

If you just want to build and run the demo:

``` bash
g++ semLib.cpp semLibDemo.cpp -o semDemo -pthread
```

### With Warnings and Debugging

Recommended during development:

``` bash
g++ -Wall -Wextra -g semLib.cpp semLibDemo.cpp -o semDemo -pthread
```

------------------------------------------------------------------------

## 4. Running the Demo

After compilation:

``` bash
./semDemo
```

You should see the four demonstration sections: 1. Binary semaphore
test\
2. Counting semaphore test\
3. Mutex protecting a shared counter\
4. Timeout behavior

------------------------------------------------------------------------

## 5. Optional: Separate Compilation

If you want to build as a small library and then link:

``` bash
g++ -c semLib.cpp -o semLib.o
g++ -c semLibDemo.cpp -o semLibDemo.o
g++ semLib.o semLibDemo.o -o semDemo -pthread
```

------------------------------------------------------------------------
