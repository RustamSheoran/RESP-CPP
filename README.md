# RESP-CPP: Event-Driven TCP Server

A high-performance, non-blocking TCP server written in modern **C++17** using direct **Linux POSIX socket APIs** and the **`epoll`** event notification facility.

This project implements a single-threaded network engine that currently functions as a concurrent, full-duplex TCP echo server with asynchronous buffering—serving as the core network foundation for a Redis-compatible protocol engine.

---

## ⚡ Current Features & Capabilities

- **Single-Threaded Reactor Core:** Uses Linux `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`) to service thousands of concurrent client connections on a single thread with 0% idle CPU utilization.
- **Non-Blocking POSIX Sockets:** Listening and client sockets are set to non-blocking mode (`O_NONBLOCK`) via `fcntl`, completely eliminating head-of-line blocking from slow or idle clients.
- **Per-Connection State Isolation:** Each connected client is tracked in an isolated `ClientState` structure with dedicated incoming (`read_buf`) and outgoing (`write_buf`) buffers.
- **Dynamic Write Readiness (`EPOLLOUT`):** Automatically registers `EPOLLOUT` when outgoing data cannot be flushed in a single non-blocking `write()`, and deregisters it once the buffer drains to avoid CPU busy-spinning.
- **Graceful Lifecycle Management:** Handles client disconnects (EOF) and socket errors cleanly by removing file descriptors from the epoll set and freeing resources.

---

## 📁 Project Structure

```text
resp-cpp/
├── Makefile          # Build configuration (C++17, -O2, -Wall, -Wextra)
├── README.md         # Current project documentation
├── ROADMAP.md        # Long-term architecture and evolution roadmap
└── src/
    ├── main.cpp      # Server entry point (configures port 6380)
    ├── server.h      # Server class interface and epoll constants
    ├── server.cpp    # Socket setup, epoll event loop, accept/read/write handlers
    ├── client.h      # ClientState struct (fd, read_buf, write_buf)
    ├── utils.h       # Utility declarations
    └── utils.cpp     # POSIX socket utilities (set_nonblocking)
```

---

## 🛠️ Build & Installation

### Requirements
- Linux OS (kernel with `epoll` support)
- `g++` (C++17 support)
- `make`

### Building

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

The server listens for incoming TCP connections on port **`6380`**.

---

## 🧪 Testing

### 1. Echo Verification
Connect using `netcat`:

```bash
nc localhost 6380
```

Type any text and hit Enter:
```text
hello
hello
systems programming
systems programming
```
The server receives the data into the client's read buffer and writes it back to the client.

### 2. Multi-Client Concurrency
Open two separate terminal windows:

```bash
# Terminal 1:
nc localhost 6380

# Terminal 2:
nc localhost 6380
```

1. Leave **Terminal 1** connected without sending any data.
2. In **Terminal 2**, type a message and press Enter.
3. **Result:** Terminal 2 receives its echo immediately. Because sockets are non-blocking and managed by `epoll`, the idle client in Terminal 1 does not block Terminal 2.

---

## ⚙️ Technical Design Decisions

- **Single-Threaded Event Loop:** Eliminates the memory overhead of thread-per-connection architectures (where each thread takes 2MB–8MB stack space) and avoids mutex locks, race conditions, and context-switching overhead.
- **Level-Triggered `epoll`:** Utilizes standard level-triggered epoll notifications for predictable socket draining and safe event handling.
- **Asynchronous Output Buffering:** Ensures data is never dropped if a client socket's kernel send buffer fills up (`EAGAIN`); remaining bytes are queued in `write_buf` until `EPOLLOUT` signals write readiness.

---

## 📚 References

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Build Your Own Redis](https://build-your-own.org/redis/)
