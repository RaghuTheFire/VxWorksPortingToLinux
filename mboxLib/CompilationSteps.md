# Compilation Steps

This document explains how to compile and run the Mailbox Library (`mboxLib`) and its demo program.

---

## Requirements

- **Compiler**: A C++11 compatible compiler (e.g., `g++`, `clang++`)
- **Operating System**: Linux, macOS, or Windows with POSIX thread support
- **Dependencies**: Standard C++11 libraries (`<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>`, `<chrono>`)

---

## Building the Demo

Run the following command in the project root directory:

```bash
g++ -std=c++11 -pthread mboxLib.cpp mboxLibDemo.cpp -o mboxDemo
```

### Explanation of Flags
- `-std=c++11` → Ensures C++11 support  
- `-pthread`   → Links POSIX threads for multithreading  
- `mboxLib.cpp` → Source file for the mailbox library implementation  
- `mboxLibDemo.cpp` → Demo application demonstrating mailbox usage  
- `-o mboxDemo` → Names the output binary `mboxDemo`  

---

## Running the Demo

After building, run the demo:

```bash
./mboxDemo
```

### Expected Output
- Producers will generate messages.  
- Consumers will retrieve and display messages.  
- The program demonstrates blocking, timeout, and non-blocking behavior.  
- At the end, the mailbox is deleted and resources are cleaned up.  

---

## Cleaning Up

To remove the compiled binary:

```bash
rm mboxDemo
```

---

## Notes
- On **Windows**, use MinGW or WSL (Windows Subsystem for Linux) with the same compilation command.  
- On **macOS/Linux**, the given command should work directly if `g++` is installed.  
