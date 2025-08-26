# Message Queue Library (msgQLib)

## Overview

This project provides a **VxWorks-style message queue library** with
support for both **FIFO** and **priority-based** message ordering. It
implements a **POSIX-compatible API** using `pthreads`, allowing it to
be used in Linux or other POSIX-compliant systems as a lightweight
replacement for VxWorks message queues.

The repository also includes a **demo application** that illustrates how
to create queues, send and receive messages, handle priorities, and
demonstrate timeout behavior.

------------------------------------------------------------------------

## Features

-   **Two queue modes**
    -   **FIFO** -- messages are received in the order they were sent.\
    -   **Priority-based** -- higher priority messages are delivered
        first.\
-   **Configurable capacity** -- define max messages and max message
    size when creating a queue.\
-   **Thread-safe** -- built with mutexes and condition variables.\
-   **Timeouts supported** -- blocking, non-blocking, and tick-based
    timeouts.\
-   **API compatible with VxWorks**
    -   `msgQCreate`\
    -   `msgQDelete`\
    -   `msgQSend`\
    -   `msgQReceive`\
    -   `vxTicksPerSecondGet` (overridable tick provider)

------------------------------------------------------------------------

## File Structure

-   **msgQLib.h** -- Public API header\
-   **msgQLib.cpp** -- Implementation of the library\
-   **msgQLibDemo.cpp** -- Demo application showcasing FIFO, priority,
    and timeout queues

------------------------------------------------------------------------

## Installation

### Prerequisites

-   **C++11 or later**\
-   **POSIX Threads (pthreads)**

### Build Instructions

Clone the repository and build using `g++`:

``` bash
g++ -o msgQDemo msgQLib.cpp msgQLibDemo.cpp -lpthread
```

This will produce an executable named **`msgQDemo`**.

------------------------------------------------------------------------

## Usage

### Creating a Queue

``` c
MSG_Q_ID q = msgQCreate(10, sizeof(my_message_t), MSG_Q_FIFO);
```

### Sending a Message

``` c
my_message_t msg = { .id = 1, .priority = 0 };
msgQSend(q, &msg, sizeof(msg), 100, 0);  // send with 1 second timeout
```

### Receiving a Message

``` c
my_message_t received;
int bytes = msgQReceive(q, &received, sizeof(received), 100);
```

### Deleting a Queue

``` c
msgQDelete(q);
```

------------------------------------------------------------------------

## Running the Demo

Run the compiled demo program:

``` bash
./msgQDemo
```

It will run through three scenarios: 1. **FIFO Queue Demo** --
producer/consumer with standard ordering.\
2. **Priority Queue Demo** -- demonstrates priority-based message
delivery.\
3. **Timeout Demo** -- shows send/receive timeout behavior.

------------------------------------------------------------------------

## Testing

The included demo (`msgQLibDemo.cpp`) doubles as a basic test suite. It
verifies: - Correct FIFO ordering\
- Correct priority-based ordering\
- Timeout handling for full/empty queues

To extend testing, you can add more producer/consumer threads or
increase message loads to stress-test the implementation.

------------------------------------------------------------------------

## Contributing

Contributions are welcome. If you'd like to improve or extend the
library: 1. Fork the repository.\
2. Create a feature branch (`git checkout -b feature/my-new-feature`).\
3. Commit your changes with clear messages.\
4. Submit a Pull Request.

Suggestions for improvement: - Add support for message queue statistics
(e.g., number of messages waiting).\
- Expand unit testing with `gtest`.\
- Add CMake build system support.

------------------------------------------------------------------------

## Customization

The system tick provider can be overridden by redefining:

``` c
int vxTicksPerSecondGet(void) {
    return 1000; // custom tick rate (1ms per tick)
}
```

------------------------------------------------------------------------

## License

This project is released under the **MIT License**. You are free to use,
modify, and distribute it in your own projects.
