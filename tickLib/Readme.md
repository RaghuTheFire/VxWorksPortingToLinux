# tickLib

A lightweight **VxWorks-like system tick library** for Linux, written in C++.  
It emulates the VxWorks `tickLib` API, providing a monotonic tick counter and configurable tick rate.

---

## Download

clone it with git:

```bash
git clone https://github.com/<your-username>/<repo-name>.git
cd <repo-name>
```

---

## Build

Using g++:

```bash
# Default (steady_clock backend)
g++ -std=c++11 tickLib.cpp tickLibDemo.cpp -o tickDemo -pthread

# POSIX clock backend
g++ -std=c++11 tickLib.cpp tickLibDemo.cpp -o tickDemo -pthread -DUSE_POSIX_CLOCK
```

---

## Run

```bash
./tickDemo
```

Example output:

```
Initializing tickLib...
System clock rate = 60 ticks/sec
Sleeping 2 seconds...
Tick after 2s = 120
Sleeping another 3 seconds...
Tick after 5s total = 300
Elapsed ticks = 180
Changing system clock rate to 200 ticks/sec...
New clock rate = 200 ticks/sec
Sleeping 1 second...
Tick now = 500
Demo finished.
```

---

## Files

- `tickLib.h` â€” public API
- `tickLib.cpp` â€” implementation
- `tickLibDemo.cpp` â€” demo program

---

## License

MIT License. See [LICENSE](LICENSE) for details.
