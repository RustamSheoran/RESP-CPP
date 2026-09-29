# RESP-CPP

A high-performance, non-blocking event-driven TCP server and in-memory key-value engine written in C++17, following the architectural progression outlined in [ROADMAP.md](file:///home/rustam/Projects/resp-cpp/ROADMAP.md).

---

## 📌 Project Status: Phases 1 to 4 Implemented

According to the milestones in [ROADMAP.md](file:///home/rustam/Projects/resp-cpp/ROADMAP.md), the core foundation is fully implemented in the main server codebase:

| Phase | Milestone | Implementation Details |
|---|---|---|
| **Phase 1** | **Foundations of Linux I/O** | Direct POSIX system calls (`read`, `write`, `close`), raw byte buffer handling, and return-value error checking (`0` for EOF/disconnect, `-1` for errors). |
| **Phase 2** | **The Simplest TCP Connection** | Full IPv4 TCP socket lifecycle (`socket` $\to$ `bind` $\to$ `listen` $\to$ `accept`), `SO_REUSEADDR` configuration, and network byte order conversion (`htons`). |
| **Phase 3** | **Multi-Client Connection Handling** | Continuous connection management supporting multiple client connections without terminating after a single request. |
| **Phase 4** | **Non-Blocking Sockets (`O_NONBLOCK`)** | Socket file descriptors set to non-blocking via `fcntl(fd, F_SETFL, flags | O_NONBLOCK)` in [`utils.cpp`](file:///home/rustam/Projects/resp-cpp/src/utils.cpp). Handling asynchronous reads and non-blocking state transitions. |

> **Note on Architecture:** The server already incorporates the single-threaded event loop and per-client buffering foundations, ensuring readiness for application-layer framing (Phase 6) and the Redis Serialization Protocol (Phase 10).

---

## 📁 Project Structure

```text
resp-cpp/
├── Makefile          # Build configuration (C++17, -Wall -Wextra -O2)
├── README.md         # Project documentation and guide
├── ROADMAP.md        # Complete 11-Phase engineering roadmap
└── src/
    ├── main.cpp      # Server entry point and configuration
    ├── server.h      # Server class declaration and epoll loop interface
    ├── server.cpp    # Socket lifecycle, connection acceptance, and I/O handlers
    ├── client.h      # ClientState structure tracking per-connection read/write buffers
    ├── utils.h       # Utility declarations
    └── utils.cpp     # Low-level POSIX utilities (set_nonblocking)
```

---

## 🛠️ Building the Project

Requirements:
- Linux OS (kernel with `epoll` support)
- `g++` supporting C++17
- `make`

To build the executable:

```bash
make
```

The compiled binary will be placed at `build/resp-server`.

To clean build artifacts:

```bash
make clean
```

---

## 🚀 Running the Server

Start the server:

```bash
./build/resp-server
```

The server will listen on port **`6380`**.

---

## 🧪 Testing & Verification

### 1. Connecting via Netcat (`nc`)
In a separate terminal, connect to the running server:

```bash
nc localhost 6380
```

Type any text and hit Enter:
```text
hello
hello
```
The server will echo back your message.

### 2. Multi-Client Concurrency Test
Open two separate terminals and connect both:

```bash
# Terminal 1:
nc localhost 6380

# Terminal 2:
nc localhost 6380
```

Send messages from both terminals. Because sockets are non-blocking and event-driven, both clients communicate simultaneously without blocking each other.

---

## 📚 Study References & Documentation

1. **[ROADMAP.md](file:///home/rustam/Projects/resp-cpp/ROADMAP.md)**: Comprehensive 11-phase systems programming roadmap designed for MAANG-tier C++ roles.
2. **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)**: The definitive guide to POSIX socket programming in C/C++.
3. **[Build Your Own Redis](https://build-your-own.org/redis/)**: Step-by-step architectural guide to building an event-driven Redis clone.
