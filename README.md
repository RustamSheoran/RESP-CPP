# RESP-CPP: In-Memory Event-Driven Redis Engine

A high-performance, single-threaded, non-blocking Redis-compatible key-value store written in modern **C++17** using direct **Linux POSIX socket APIs** and the **`epoll`** event notification facility.

This project implements an event-driven networking engine coupled with an in-memory dictionary and a zero-copy command parser utilizing C++17 `std::string_view` to process Redis commands with minimal heap allocations.

---

## ⚡ Key Features & Supported Commands

- **Supported Redis Commands (RESP Output):**
  - `PING [msg]` $\to$ Returns `+PONG\r\n` or bulk string `$<len>\r\n<msg>\r\n`.
  - `SET <key> <value>` $\to$ Stores key-value pair in memory, returns `+OK\r\n`.
  - `GET <key>` $\to$ Returns `$<len>\r\n<value>\r\n`, or `$-1\r\n` (nil) if not found.
  - `DEL <key> [<key2> ...]` $\to$ Deletes keys, returns integer count `:<count>\r\n`.
- **Zero-Copy Command Tokenizer:** Uses C++17 `std::string_view` to slice command tokens directly from client buffers without allocating temporary heap strings during parsing.
- **Single-Threaded Reactor Core:** Uses Linux `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`) to service thousands of concurrent client connections on a single thread with 0% idle CPU utilization.
- **Non-Blocking POSIX Sockets:** Listening and client sockets are set to non-blocking mode (`O_NONBLOCK`) via `fcntl`, completely eliminating head-of-line blocking from slow or idle clients.
- **Application Message Framing:** Robust accumulation and delimiter-based slicing (`\n`) handling fragmented packets and coalesced commands over TCP byte streams.
- **Dynamic Write Readiness (`EPOLLOUT`):** Automatically registers `EPOLLOUT` when outgoing responses cannot be flushed in a single non-blocking `write()`, and deregisters it once the buffer drains to avoid CPU busy-spinning.
- **Resilient Connection Lifecycle & `SIGPIPE` Shield:** Explicitly ignores `SIGPIPE` at startup to prevent client crashes from killing the server, and guarantees descriptor leak prevention on disconnects.

---

## 📁 Project Structure

```text
resp-cpp/
├── Makefile          # Build configuration (C++17, -O2, -Wall, -Wextra)
├── README.md         # Current project documentation
├── ROADMAP.md        # Long-term architecture and evolution roadmap
└── src/
    ├── main.cpp      # Server entry point (SIGPIPE shield & port 6380 config)
    ├── protocol.h    # Zero-copy command parser interface
    ├── protocol.cpp  # In-memory database (hash map) & Redis command executor
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

### 1. Interactive Redis Commands via Netcat (`nc`)
Connect to the server:

```bash
nc localhost 6380
```

Execute Redis commands:
```text
PING
+PONG

SET user rustam
+OK

GET user
$6
rustam

SET greeting "hello world"
+OK

GET greeting
$11
hello world

DEL user
:1

GET user
$-1
```

### 2. Multi-Client Concurrency
Open two separate terminal windows:

```bash
# Terminal 1:
nc localhost 6380

# Terminal 2:
nc localhost 6380
```

1. In **Terminal 1**, leave the connection idle or set a key: `SET counter 100`.
2. In **Terminal 2**, read the key: `GET counter`.
3. **Result:** Terminal 2 immediately receives `$3\r\n100\r\n`. Both clients run concurrently on the single-threaded epoll reactor without blocking.

---

## ⚙️ Technical Design Decisions

- **`std::unordered_map` over `std::map`:** Used for both connection lookup (`int fd -> ClientState`) and the in-memory database (`std::string -> std::string`) to guarantee amortized $O(1)$ constant-time operations rather than $O(\log N)$ tree traversals.
- **`std::string_view` for Tokenization:** Slices incoming command tokens without copying memory. Data ownership is transferred to `std::string` only when permanently inserting a key-value pair into the database.
- **Single-Threaded Event Loop:** Eliminates the memory overhead of thread-per-connection architectures (where each thread takes 2MB–8MB stack space) and avoids mutex locks, race conditions, and context-switching overhead.
- **Asynchronous Output Buffering:** Ensures data is never dropped if a client socket's kernel send buffer fills up (`EAGAIN`); remaining bytes are queued in `write_buf` until `EPOLLOUT` signals write readiness.

---

## 📚 References

- [Redis Protocol Specification (RESP)](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Build Your Own Redis](https://build-your-own.org/redis/)
