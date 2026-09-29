# RESP-CPP: In-Memory Event-Driven Redis Engine

A high-performance, single-threaded, non-blocking Redis-compatible key-value store written in modern **C++17** using direct **Linux POSIX socket APIs** and the **`epoll`** event notification facility.

This project implements an event-driven reactor core paired with an in-memory dictionary, a zero-copy command parser utilizing C++17 `std::string_view` to process Redis commands with minimal heap allocations, socket-level latency optimizations (`TCP_NODELAY`), dynamic backpressure buffering, and production-grade Slowloris DoS protection.

---

## ⚡ Key Features & Supported Commands

- **Supported Redis Commands (Strict RESP Protocol):**
  - `PING [msg]` $\to$ Returns `+PONG\r\n` or bulk string `$<len>\r\n<msg>\r\n`.
  - `SET <key> <value>` $\to$ Stores key-value pair in memory, returns `+OK\r\n` (supports spaces and quotes).
  - `GET <key>` $\to$ Returns `$<len>\r\n<value>\r\n`, or `$-1\r\n` (nil) if key does not exist.
  - `DEL <key> [<key2> ...]` $\to$ Deletes keys, returns integer count `:<count>\r\n`.
  - `COMMAND` $\to$ Returns empty array `*0\r\n` for seamless handshake compatibility with standard Redis clients and CLI tools.
  - `QUIT` $\to$ Returns `+OK\r\n` for graceful client teardown.
- **Zero-Copy Command Tokenizer:** Uses C++17 `std::string_view` to slice command tokens directly from client buffers without allocating temporary heap strings during parsing.
- **Single-Threaded Reactor Core:** Uses Linux `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`) to service thousands of concurrent client connections on a single thread with 0% idle CPU utilization.
- **Low-Latency TCP Socket Tuning (`TCP_NODELAY`):** Disables Nagle's algorithm on all accepted client sockets, eliminating 40ms delayed-ACK stalls on request-response cycles.
- **Dynamic Write Backpressure (`EPOLLOUT`):** Automatically registers `EPOLLOUT` when outgoing responses cannot be completely flushed in a single non-blocking `write()`, and deregisters it once the buffer drains to prevent CPU busy-spinning.
- **Slowloris DoS Defense (Hardening):** Strictly caps maximum query length (`MAX_QUERY_LEN = 300 bytes`) to prevent memory exhaustion attacks from malicious slow-streaming clients.
- **Resilient Connection Lifecycle & `SIGPIPE` Shield:** Explicitly ignores `SIGPIPE` (`signal(SIGPIPE, SIG_IGN)`) to prevent broken pipe signals from crashing the server when clients disconnect abruptly.
- **Zero Console Overhead in Production:** Hot-path debug logging is conditionally compiled behind `#ifdef VERBOSE`, preventing console I/O from bottlenecking reactor throughput.
- **Memory Sanitized:** Built and verified with Google's AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) (`-fsanitize=address,undefined`), guaranteeing zero memory leaks and strict memory safety.

---

## 📁 Project Structure

```text
resp-cpp/
├── Makefile          # Multi-target build system (release, asan, debug)
├── README.md         # Architecture, design decisions, and benchmarks
├── ROADMAP.md        # 11-Phase systems engineering roadmap
├── benchmark.py      # Async multi-client load testing and latency suite
└── src/
    ├── main.cpp      # Server entry point (SIGPIPE shield & port 6380 config)
    ├── protocol.h    # Zero-copy command parser interface
    ├── protocol.cpp  # In-memory database (hash map) & Redis command executor
    ├── server.h      # Server class interface and epoll constants
    ├── server.cpp    # Socket lifecycle, epoll loop, non-blocking I/O handlers
    ├── client.h      # ClientState struct (fd, read_buf, write_buf)
    ├── utils.h       # Socket utility declarations
    └── utils.cpp     # POSIX socket utilities (fcntl O_NONBLOCK)
```

---

## 🛠️ Build & Installation

### Requirements
- Linux OS (kernel with `epoll` support)
- `g++` (C++17 support)
- `make`

### Build Targets

```bash
# Maximum performance release build (-O3 -march=native -DNDEBUG)
make release
# or simply:
make

# Memory and runtime auditing build (AddressSanitizer + UndefinedBehaviorSanitizer)
make asan

# Debug build with verbose connection logging enabled (-DVERBOSE)
make debug
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

The server binds to `0.0.0.0` and listens for incoming TCP connections on port **`6380`**.

---

## 🧪 Testing & Verification

### 1. Interactive Redis Commands via Netcat (`nc`)

Connect to the server:

```bash
nc localhost 6380
```

Execute Redis commands:
```text
PING
+PONG

PING "hello world"
$11
hello world

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

COMMAND
*0

QUIT
+OK
```

### 2. Multi-Client Concurrency Test

Open two separate terminal windows:

```bash
# Terminal 1:
nc localhost 6380

# Terminal 2:
nc localhost 6380
```

1. In **Terminal 1**, set a key: `SET counter 100`.
2. In **Terminal 2**, read the key: `GET counter`.
3. **Result:** Terminal 2 immediately receives `$3\r\n100\r\n`. Both clients run concurrently on the single-threaded epoll reactor without blocking.

### 3. Slowloris DoS Defense Verification

Send an abusive query exceeding the 300-byte threshold without a newline delimiter:

```bash
python3 -c "import socket; s = socket.socket(); s.connect(('127.0.0.1', 6380)); s.sendall(b'A'*350); print('Disconnected:', len(s.recv(1024)) == 0)"
```

Output:
```text
Disconnected: True
```

The server immediately drops the abusive connection and releases its file descriptor.

---

## 📊 Benchmarks (Over 230,000+ QPS)

A custom asynchronous benchmark suite (`benchmark.py`) is included to evaluate throughput and tail latency under heavy connection load and pipelining.

### 1. Pipelined High-Throughput Test (50,000 requests, pipeline depth 16)

```bash
python3 benchmark.py --clients 100 --requests 50000 --pipeline 16
```

```text
============================================================
  RESP-CPP Engine Benchmark Suite
============================================================
Target:               127.0.0.1:6380
Concurrent Clients:   100
Pipeline Depth:       16
Total Requests:       50,000 (Alternating SET & GET)
============================================================
[SUCCESS] Completed 50,000 requests across 100 clients in 0.22 seconds.
[RESULT]  Throughput:  230,903 QPS (Requests/sec)
[LATENCY] Avg: 0.40 ms | p50: 0.40 ms | p95: 0.50 ms | p99: 0.58 ms
============================================================
```

### 2. Massive Concurrency Stress Test (1,000 Concurrent Clients, 100,000 requests)

```bash
python3 benchmark.py --clients 1000 --requests 100000 --pipeline 16
```

```text
============================================================
[SUCCESS] Completed 100,000 requests across 1,000 clients in 0.60 seconds.
[RESULT]  Throughput:  165,386 QPS (Requests/sec)
[LATENCY] Avg: 4.37 ms | p50: 4.08 ms | p95: 4.22 ms | p99: 15.22 ms
============================================================
```

---

## ⚙️ Technical Design Decisions

- **`std::unordered_map` over `std::map`:** Used for both connection state lookup (`int fd -> ClientState`) and the in-memory database (`std::string -> std::string`) to guarantee amortized $O(1)$ constant-time operations rather than $O(\log N)$ red-black tree traversals.
- **`std::string_view` for Zero-Copy Tokenization:** Incoming client request buffers are parsed without allocating temporary strings on the heap. Ownership is transferred to `std::string` only when storing entries in the hash map.
- **Single-Threaded Epoll Reactor:** Eliminates the heavy memory footprint of thread-per-connection designs (where each thread incurs 2MB–8MB stack allocation) and avoids locking overhead, mutex contention, and kernel context-switching latency.
- **`TCP_NODELAY` (Disabling Nagle's Algorithm):** Nagle's algorithm aggregates small TCP packets until an ACK is received. In a request-response protocol, this frequently triggers a 40ms delayed-ACK penalty. Setting `TCP_NODELAY` ensures responses are transmitted immediately.
- **Dynamic Write Buffering (`EPOLLOUT` Backpressure):** Sockets are never blocked on large writes. Responses are appended to `ClientState::write_buf`, written non-blockingly, and dynamically watched with `EPOLLOUT` only when partially written, preventing busy polling.
- **AddressSanitizer (ASan) & UBSan Verification:** The entire codebase compiles cleanly under `-fsanitize=address,undefined` to guarantee no use-after-free, buffer overflows, or memory leaks occur during concurrent connections.

---

## 📚 References

- [Redis Protocol Specification (RESP)](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Linux man pages: epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Google Sanitizers Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [Build Your Own Redis](https://build-your-own.org/redis/)
