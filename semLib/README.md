# Semaphore Library (semLib)

A lightweight C library that provides binary, counting, and mutex semaphores with a simple API.  
It wraps POSIX semaphores and pthread mutexes in a consistent interface, inspired by VxWorks-style semaphore handling.

---

## Features

- **Binary semaphores** – classic lock/unlock behavior  
- **Counting semaphores** – allow multiple concurrent accesses  
- **Mutex semaphores** – for mutual exclusion with recursive acquisition protection  
- **Timeout support** – acquire semaphores with blocking, non-blocking, or time-limited waits  
- **FIFO/Priority queue options** – placeholders for compatibility (priority not implemented yet)  

---

## File Structure

- `semLib.h` – Public API header  
- `semLib.cpp` – Implementation of semaphore functions  
- `semLibDemo.cpp` – Demo program showcasing usage  

---

## API Overview

```c
SEM_ID semBCreate(int options, int initialState);   // Binary semaphore
SEM_ID semCCreate(int options, int initialCount);   // Counting semaphore
SEM_ID semMCreate(int options);                     // Mutex semaphore

int semTake(SEM_ID sem, int ticks);   // Acquire
int semGive(SEM_ID sem);              // Release
int semDelete(SEM_ID sem);            // Destroy
```

**Timeout behavior in `semTake`:**
- `-1` → wait indefinitely  
- `0` → non-blocking, return immediately  
- `>0` → wait for `ticks` (approx. 10ms per tick)  

---

## Minimal Usage Example

```c
#include "semLib.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

SEM_ID binarySem;

void* worker(void* arg) {
    semTake(binarySem, -1); // wait forever
    printf("Thread %ld: acquired semaphore\n", (long)arg);
    sleep(1); // simulate work
    semGive(binarySem);
    printf("Thread %ld: released semaphore\n", (long)arg);
    return NULL;
}

int main() {
    binarySem = semBCreate(SEM_Q_FIFO, 1); // initially available

    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, (void*)1);
    pthread_create(&t2, NULL, worker, (void*)2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    semDelete(binarySem);
    return 0;
}
```

Compile:

```bash
gcc -o test main.c semLib.cpp -lpthread
```

Run:

```
Thread 1: acquired semaphore
Thread 1: released semaphore
Thread 2: acquired semaphore
Thread 2: released semaphore
```

---

## Running the Full Demo

The repository also includes a full demo (`semLibDemo.cpp`) that showcases:

1. **Binary Semaphore** – ensures only one thread runs at a time  
2. **Counting Semaphore** – allows limited parallelism (e.g., 2 threads concurrently)  
3. **Mutex Semaphore** – protects a shared counter against race conditions  
4. **Timeout Handling** – demonstrates acquiring with a deadline  

Build and run:

```bash
gcc -o semDemo semLib.cpp semLibDemo.cpp -lpthread
./semDemo
```

---

## Requirements

- POSIX-compliant system (Linux, macOS, etc.)  
- `pthread` library  

---

## Notes

- Queueing policies (`SEM_Q_FIFO`, `SEM_Q_PRIORITY`) are defined but only FIFO is effective.  
- Always ensure no threads are blocked on a semaphore before calling `semDelete`.  

---

## License

Bharat License. Free to use and modify.
