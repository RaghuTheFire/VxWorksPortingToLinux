# Mailbox Library (mboxLib)

A lightweight C++11 mailbox implementation for inter-task and inter-thread communication.  
This library provides a thread-safe message-passing mechanism with blocking, non-blocking, and timeout support. It’s designed for real-time and embedded systems but can also be used in standard multithreaded applications.

---

## Features

- **Thread-safe mailbox** with configurable capacity  
- **Blocking and non-blocking send/receive**  
- **Timeout support** (expressed in system ticks, assuming 1000 ticks per second)  
- **Clean shutdown handling** — waiting threads are unblocked when a mailbox is deleted  
- Minimal C++11 dependencies (mutex, condition_variable, atomic, chrono, thread)

---

## API Overview

Defined in [`mboxLib.h`](mboxLib.h):

```cpp
// Create a mailbox
MBOX_ID mboxCreate(int capacity);

// Delete a mailbox
int mboxDelete(MBOX_ID mboxId);

// Send a message (blocking with timeout)
int mboxSend(MBOX_ID mboxId, void* msg, int timeoutTicks);

// Receive a message (blocking with timeout)
int mboxReceive(MBOX_ID mboxId, void** pMsg, int timeoutTicks);
```

### Notes
- Messages are passed as pointers (`void*`).  
- The sender retains ownership until the message is received.  
- The receiver assumes ownership and is responsible for freeing or handling the memory.  
- `timeoutTicks`:
  - `-1` = wait indefinitely  
  - `0`  = non-blocking (immediate return if not possible)  
  - `N > 0` = wait for `N` ticks  

---

## Demo Application

[`mboxLibDemo.cpp`](mboxLibDemo.cpp) shows how to use the library in a multithreaded program.  

### Highlights
- Multiple **producer threads** send messages into the mailbox.  
- Multiple **consumer threads** receive and process messages.  
- Demonstrates blocking, timeout, and **non-blocking send/receive**.  
- Clean shutdown with `mboxDelete`.

---

## Building

You’ll need a C++11 compatible compiler.  

```bash
g++ -std=c++11 -pthread mboxLib.cpp mboxLibDemo.cpp -o mboxDemo
```

---

## Running the Demo

```bash
./mboxDemo
```

You’ll see producers generating messages, consumers retrieving them, and a demonstration of non-blocking operations.

Example output snippet:

```
Mailbox Library Demo Application
=================================
Created mailbox with capacity 10
Producer 0 started, sending 5 messages
Consumer 0 started, expecting 5 messages
Producer 0 sent message 0
Consumer 0 received: [0] Message 0 from producer 0 (timestamp: ...)
...
=== Non-blocking operation demo ===
No message available (as expected) in non-blocking receive
Non-blocking send succeeded
Mailbox deleted, demo completed
```

---

## Use Cases

- Real-time operating systems (RTOS) task communication  
- Multithreaded applications needing lightweight message queues  
- Embedded systems with constrained resources  

---

## License

MIT License. See [LICENSE](LICENSE) for details.  
