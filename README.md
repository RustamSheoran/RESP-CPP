# RESP-CPP: In-Memory Event-Driven Redis Engine

> ✍️ **Blog Post by the Author:**  
> Read my full backstory, systems design breakdown, and engineering journey on Medium:  
> 🔗 [**I Was Bored and Had Free Time, So I Built Redis From Scratch**](https://medium.com/@rustam98137/i-was-bored-and-had-free-time-so-i-built-redis-from-scratch-957a17b697c5)

A high-performance, single-threaded, non-blocking Redis-compatible (subset: `PING`, `SET`, `GET`, `DEL`, `COMMAND`, `INFO`, `QUIT`) key-value store written in modern **C++17** using direct **Linux POSIX socket APIs** and the **`epoll`** event notification facility.

This project implements an event-driven reactor core paired with an in-memory dictionary, a dual-mode protocol parser supporting both the official **RESP Multibulk Array protocol** (native `redis-cli` and standard drivers) and **quote-aware inline commands** (`nc`/`telnet`), socket-level latency optimizations (`TCP_NODELAY`), dynamic backpressure buffering, `EMFILE` connection exhaustion protection, rate-limited idle connection sweeps, and defensive protocol bounds checking.

---

## ⚡ Key Features & Supported Commands

- **Dual-Mode Protocol Engine:**
  - **RESP Multibulk Arrays (`*N\r\n$<len>\r\n...`):** Full binary-safe support for standard Redis client drivers and the official `redis-cli` tool.
  - **Quote-Aware Inline Commands:** Supports spaces inside quoted keys and values (`"hello world"`) with escape sequences (`\"`, `\\`) for direct testing via `nc` or `telnet`.
- **Supported Redis Commands (Strict RESP Output):**
  - `PING [msg]` $\to$ Returns `+PONG\r\n` or bulk string `$<len>\r\n<msg>\r\n`.
  - `SET <key> <value>` $\to$ Stores key-value pair in memory, returns `+OK\r\n` (strictly validates 2 arguments; rejects unsupported flags with standard syntax error; supports spaces and quotes in both keys and values).
  - `GET <key>` $\to$ Returns `$<len>\r\n<value>\r\n`, or `$-1\r\n` (nil) if key does not exist.
  - `DEL <key> [<key2> ...]` $\to$ Deletes one or more keys, returns integer count `:<count>\r\n`.
  - `COMMAND` / `INFO` $\to$ Returns `*0\r\n` / `$0\r\n\r\n` for seamless handshake compatibility with standard Redis clients and CLI tools.
  - `QUIT` $\to$ Flushes `+OK\r\n` and initiates clean TCP socket teardown.
- **Single-Threaded Reactor Core:** Uses Linux `epoll` (`epoll_create1`, `epoll_ctl`, `epoll_wait`) to service thousands of concurrent client connections on a single thread with 0% idle CPU utilization.
- **Low-Latency TCP Socket Tuning (`TCP_NODELAY`):** Disables Nagle's algorithm on all accepted client sockets, eliminating 40ms delayed-ACK stalls on request-response cycles.
- **`EMFILE` Spin Prevention:** Reserves a dedicated spare file descriptor (`/dev/null`) to drain connection backlog queues when the process hits descriptor limits (`ulimit -n`), curing the classic level-triggered epoll 100% CPU spin.
- **Rate-Limited Idle Connection Timeout:** Monotonically tracks client activity (`std::chrono::steady_clock`) with an idle sweep evaluated at most once per second, disconnecting connections inactive for longer than 60 seconds.
- **Dynamic Write Backpressure (`EPOLLOUT`) & Ingress Capping:** Automatically registers `EPOLLOUT` when outgoing responses cannot be completely flushed in a single non-blocking `write()`, and enforces a 10 MB backpressure write ceiling and a 10 MB read buffer ceiling to prevent memory exhaustion attacks.
- **Defensive Protocol Hardening:** Strictly bounds multibulk arguments ($\le 1,024 \times 1,024$), bulk string lengths ($\le 512\text{ MB}$), and rejects malformed lengths (e.g. `*99999999999\r\n`) with safe protocol errors and socket closure rather than unhandled memory allocation errors.
- **Resilient Connection Lifecycle & `SIGPIPE` Shield:** Explicitly ignores `SIGPIPE` (`signal(SIGPIPE, SIG_IGN)`) to prevent broken pipe signals from crashing the server when clients disconnect abruptly.
- **Zero Console Overhead in Production:** Hot-path debug logging is conditionally compiled behind `#ifdef VERBOSE`, preventing console I/O from bottlenecking reactor throughput.
- **Memory Sanitized:** Built and verified with Google's AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) (`-fsanitize=address,undefined`), with zero errors or leaks found under ASan/UBSan across all test and endurance runs.

---

## 📁 Project Structure

```text
resp-cpp/
├── Makefile          # Multi-target build system (release, asan, debug)
├── README.md         # Architecture, design decisions, and documentation
├── ROADMAP.md        # 11-Phase systems engineering roadmap
├── docs/             # Line-by-line documentation for each source file
│   ├── README.md
│   ├── client.h.md
│   ├── main.cpp.md
│   ├── protocol.h.md
│   ├── protocol.cpp.md
│   ├── server.h.md
│   ├── server.cpp.md
│   ├── utils.h.md
│   ├── utils.cpp.md
│   └── Makefile.md
├── logs/             # Raw benchmark execution logs and latency percentiles
│   ├── README.md
│   ├── valkey_60s.log
│   ├── resp_60s.log
│   ├── valkey_120s.log
│   ├── resp_120s.log
│   ├── valkey_500s.log
│   ├── resp_500s.log
│   ├── valkey_percentiles.log
│   └── resp_percentiles.log
└── src/
    ├── main.cpp      # Server entry point (SIGPIPE shield & port 6380 config)
    ├── protocol.h    # Protocol parser interface & ParseResult enum
    ├── protocol.cpp  # Dual-mode RESP/inline parser & in-memory key-value database
    ├── server.h      # Server class interface, constants, and epoll structures
    ├── server.cpp    # Socket lifecycle, epoll loop, non-blocking I/O handlers
    ├── client.h      # ClientState struct (fd, read_buf, write_buf, timestamps)
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

### 1. Official `redis-cli` Connection
Connect using the standard Redis command-line interface:

```bash
redis-cli -p 6380
```

Execute Redis commands:
```text
127.0.0.1:6380> ping
PONG
127.0.0.1:6380> ping "hello world"
"hello world"
127.0.0.1:6380> set "user name" "rustam"
OK
127.0.0.1:6380> get "user name"
"rustam"
127.0.0.1:6380> del "user name"
(integer) 1
127.0.0.1:6380> get "user name"
(nil)
127.0.0.1:6380> quit
OK
```

### 2. Interactive Inline Commands via Netcat (`nc`)

Connect to the server:

```bash
nc localhost 6380
```

Execute commands:
```text
PING
+PONG

PING "hello world"
$11
hello world

SET "my key" "hello world"
+OK

GET "my key"
$11
hello world

DEL "my key"
:1

GET "my key"
$-1

QUIT
+OK
```

### 3. Multi-Client Concurrency Test

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

---

## 📊 Performance Benchmarks (RESP-CPP vs. Valkey 9.1.2)

A rigorous head-to-head performance evaluation was conducted using `valkey-benchmark` comparing **RESP-CPP** against **Valkey 9.1.2** (the open-source high-performance Redis fork). Tests measured sustained throughput (QPS) and exact tail latencies across 60s, 120s, and 500s runs under strict Linux CPU core affinity isolation.

### 🖥️ Hardware Profile & Test Setup

- **Processor:** Intel® Core™ i5-4430 @ 3.00GHz (Haswell, 4 physical cores, 4 threads, 6 MB L3 cache)
- **RAM:** 16 GB DDR3 (15 GiB usable, ~12 GiB free)
- **Operating System:** Linux 7.2.5-3-omarchy x86_64
- **Affinity & Isolation:**
  - Server process (`valkey-server` or `resp-server`): Dedicated to **CPU Core 0** (`taskset -c 0`)
  - Benchmark client (`valkey-benchmark`): Dedicated to **CPU Cores 1–3** (`taskset -c 1-3`)
- **Transport & Workload:** Local loopback interface (`127.0.0.1`), unpipelined synchronous request-response (`-P 1`), 50 concurrent client connections (`-c 50`)
- **Server Configurations:**
  - **Valkey 9.1.2:** `--save "" --appendonly no --protected-mode no` (pure in-memory configuration without disk I/O)
  - **RESP-CPP:** Production build compiled with `g++ -std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG`

---

### 📈 Sustained Throughput Across Test Durations

#### 1. Run 1: 500 Seconds (Sustained Endurance Benchmark)

| Command | Valkey 9.1.2 | RESP-CPP | Difference | p50 Latency (Valkey / RESP-CPP) |
| :--- | :--- | :--- | :--- | :--- |
| **`PING_INLINE`** | 115,648 req/s | **113,118 req/s** | -2.2% | 0.215 ms / 0.223 ms |
| **`PING_MBULK`** | 115,845 req/s | **112,992 req/s** | -2.5% | 0.215 ms / 0.223 ms |
| **`SET`** | 116,218 req/s | **113,094 req/s** | -2.7% | 0.223 ms / 0.223 ms |
| **`GET`** | 116,024 req/s | **113,122 req/s** | -2.5% | 0.215 ms / 0.223 ms |

#### 2. Run 2: 120 Seconds (Hybrid Soak Benchmark)

| Command | Valkey 9.1.2 | RESP-CPP | Difference | p50 Latency (Valkey / RESP-CPP) |
| :--- | :--- | :--- | :--- | :--- |
| **`PING_INLINE`** | 115,812 req/s | **114,388 req/s** | -1.2% | 0.215 ms / 0.223 ms |
| **`PING_MBULK`** | 116,121 req/s | **114,115 req/s** | -1.7% | 0.215 ms / 0.223 ms |
| **`SET`** | 116,120 req/s | **114,356 req/s** | -1.5% | 0.223 ms / 0.223 ms |
| **`GET`** | 116,360 req/s | **115,088 req/s** | -1.1% | 0.215 ms / 0.223 ms |

#### 3. Run 3: 60 Seconds (Industry Standard Microbenchmark)

| Command | Valkey 9.1.2 | RESP-CPP | Difference | p50 Latency (Valkey / RESP-CPP) |
| :--- | :--- | :--- | :--- | :--- |
| **`PING_INLINE`** | 115,408 req/s | **114,251 req/s** | -1.0% | 0.215 ms / 0.223 ms |
| **`PING_MBULK`** | 115,589 req/s | **114,380 req/s** | -1.0% | 0.215 ms / 0.223 ms |
| **`SET`** | 116,163 req/s | **115,258 req/s** | -0.8% | 0.223 ms / 0.223 ms |
| **`GET`** | 116,483 req/s | **116,059 req/s** | -0.4% | 0.215 ms / 0.223 ms |

---

### ⏱️ High-Precision Tail Latency Distribution (p50, p87.5, p95, p99, p99.9, p99.99)

Measured at high precision (`--precision 3`) with 50 parallel clients over 10-second test intervals:

| Command | Metric | Valkey 9.1.2 | RESP-CPP | Differential |
| :--- | :--- | :--- | :--- | :--- |
| **`PING_INLINE`** | **Throughput** | 115,051.60 req/s | 115,831.10 req/s | +0.68% |
| | **p50 (Median)** | 0.215 ms | 0.215 ms | Identical (0.000 ms) |
| | **p75** | 0.231 ms | 0.239 ms | +0.008 ms |
| | **p87.5** | 0.247 ms | 0.255 ms | +0.008 ms |
| | **p95** | 0.271 ms | 0.279 ms | +0.008 ms |
| | **p99** | 0.319 ms | 0.327 ms | +0.008 ms |
| | **p99.9** | 0.479 ms | 0.767 ms | +0.288 ms |
| | **p99.99** | 0.911 ms | 1.927 ms | +1.016 ms |
| | **Max** | 1.895 ms | 1.983 ms | +0.088 ms |
| **`PING_MBULK`** | **Throughput** | 113,622.80 req/s | 115,592.60 req/s | +1.73% |
| | **p50 (Median)** | 0.215 ms | 0.215 ms | Identical (0.000 ms) |
| | **p75** | 0.231 ms | 0.231 ms | Identical (0.000 ms) |
| | **p87.5** | 0.247 ms | 0.247 ms | Identical (0.000 ms) |
| | **p95** | 0.279 ms | 0.271 ms | -0.008 ms |
| | **p99** | 0.335 ms | 0.311 ms | -0.024 ms |
| | **p99.9** | 1.855 ms | 0.767 ms | -1.088 ms |
| | **p99.99** | 3.471 ms | 2.487 ms | -0.984 ms |
| | **Max** | 5.615 ms | 4.151 ms | -1.464 ms |
| **`SET`** | **Throughput** | 113,209.20 req/s | 117,190.70 req/s | +3.52% |
| | **p50 (Median)** | 0.223 ms | 0.215 ms | -0.008 ms |
| | **p75** | 0.239 ms | 0.239 ms | Identical (0.000 ms) |
| | **p87.5** | 0.255 ms | 0.255 ms | Identical (0.000 ms) |
| | **p95** | 0.279 ms | 0.279 ms | Identical (0.000 ms) |
| | **p99** | 0.335 ms | 0.327 ms | -0.008 ms |
| | **p99.9** | 1.895 ms | 1.407 ms | -0.488 ms |
| | **p99.99** | 2.503 ms | 2.487 ms | -0.016 ms |
| | **Max** | 4.095 ms | 2.727 ms | -1.368 ms |
| **`GET`** | **Throughput** | 116,295.30 req/s | 95,191.04 req/s | -18.1% |
| | **p50 (Median)** | 0.223 ms | 0.223 ms | Identical (0.000 ms) |
| | **p75** | 0.231 ms | 0.255 ms | +0.024 ms |
| | **p87.5** | 0.247 ms | 0.303 ms | +0.056 ms |
| | **p95** | 0.271 ms | 0.551 ms | +0.280 ms |
| | **p99** | 0.303 ms | 1.903 ms | +1.600 ms |
| | **p99.9** | 0.479 ms | 3.703 ms | +3.224 ms |
| | **p99.99** | 2.343 ms | 5.743 ms | +3.400 ms |
| | **Max** | 3.175 ms | 6.799 ms | +3.624 ms |

> **Analysis on Tail Latencies, Microbenchmark Variance & Workload Limitations:**
>
> 1. **Run-to-Run Variance:** Single short (10-second) benchmark runs are susceptible to ambient noise ($\pm 2\text{--}3\%$). For instance, Valkey’s own SET throughput varied between 113,209 req/s in the 10s run and 116,218 req/s in the 500s run (a 2.7% swing). Thus, isolated differences like +0.68% or +3.52% reflect expected run-to-run variance rather than a structural speed advantage.
> 2. **Tail Latency Reality:** While median latency is identical across both servers (p50: 0.215–0.223 ms), RESP-CPP exhibits higher tail latency on inline commands at extreme percentiles (`PING_INLINE` p99.9 of 0.767 ms vs Valkey's 0.479 ms; p99.99 of 1.927 ms vs 0.911 ms).
> 3. **The GET Microbenchmark Variance & 5-Run Verification:** The initial 10s GET run measured 95,191 req/s with 3% of requests exceeding 1 ms, alongside periodic dips to ~106k in the 500s run. To verify empirical stability, five consecutive 10-second GET runs were executed under identical CPU pinning:
>    - **Run 1:** 113,766 req/s (p50: 0.223 ms)
>    - **Run 2:** 111,024 req/s (p50: 0.231 ms)
>    - **Run 3:** 114,120 req/s (p50: 0.223 ms)
>    - **Run 4:** 112,129 req/s (p50: 0.223 ms)
>    - **Run 5:** 108,065 req/s (p50: 0.239 ms)
>    - **5-Run Summary:** Min = 108,065 req/s | Max = 114,120 req/s | **Median = 112,129 req/s** (Spread: 6,055 req/s).  
>    Under C++17, looking up keys in `std::unordered_map` constructs a temporary `std::string(tokens[1])` per query, which incurs minor dynamic heap allocator contention during 100k+ QPS bursts compared to Valkey's zero-allocation C dictionary.
> 4. **Loopback Syscall Bottleneck:** Under `-P 1` (unpipelined, single-request/reply) on a single CPU core, both engines plateau around ~115k req/s regardless of command. At this concurrency, throughput is primarily constrained by Linux loopback socket system calls (`epoll_wait`, `read`, `write`) and kernel context switches rather than internal engine logic. High-concurrency workloads using pipelining (`-P 16`), large randomized keyspaces (`-r 1000000`), and variable payload sizes (`-d 256`) are necessary to stress pure reactor efficiency and cache locality.
>
> All condensed progression logs (sampled periodic snapshots), high-precision latency percentiles, and test runner artifacts are preserved in the [`logs/`](./logs/) directory.

---

## ⚙️ Technical Design Decisions

- **Dual-Mode Request Framing:** Inspects the first byte of the client buffer. If `*`, it executes binary-safe RESP Multibulk Array parsing with incomplete payload buffering (`NEED_MORE_DATA`). If not `*`, it falls back to newline-framed inline commands processed by a quote-aware tokenizer.
- **`std::unordered_map` over `std::map`:** Used for both connection state lookup (`int fd -> ClientState`) and the in-memory database (`std::string -> std::string`) to guarantee amortized $O(1)$ constant-time operations rather than $O(\log N)$ red-black tree traversals.
- **`EMFILE` Level-Triggered Spin Cure:** When a process reaches its file descriptor limit, `accept()` fails with `EMFILE`, leaving the pending socket in the kernel listen backlog and causing level-triggered epoll to fire at 100% CPU. Opening a reserved spare descriptor (`/dev/null`) allows closing it temporarily, draining and closing the excess socket, and reopening the reserve descriptor.
- **`TCP_NODELAY` (Disabling Nagle's Algorithm):** Nagle's algorithm aggregates small TCP packets until an ACK is received. In request-response protocols, this triggers a 40ms delayed-ACK penalty. Setting `TCP_NODELAY` ensures responses transmit immediately.
- **Dynamic Write Buffering (`EPOLLOUT` Backpressure):** Sockets are never blocked on large writes. Responses are appended to `ClientState::write_buf`, written non-blockingly, and dynamically watched with `EPOLLOUT` only when partially written, preventing busy polling. A 10 MB ceiling prevents unbounded memory growth.
- **Bounded Multibulk Parsing & Remote Crash Immunity:** Memory allocation for argument vectors is protected against integer overflow and excessive allocation exploits (capping argument count to $1,024 \times 1,024$, bulk strings to $512\text{ MB}$, and ingress buffers to $10\text{ MB}$). Malformed frames trigger an immediate `-ERR Protocol error` and socket termination, ensuring the reactor never crashes from memory allocation failures.
- **AddressSanitizer (ASan) & UBSan Verification:** The entire codebase compiles cleanly and was verified under `-fsanitize=address,undefined`, with no use-after-free, buffer overflows, or memory leaks detected across comprehensive concurrency and endurance test runs.

---

## 📚 References

- [I Was Bored and Had Free Time, So I Built Redis From Scratch (Medium)](https://medium.com/@rustam98137/i-was-bored-and-had-free-time-so-i-built-redis-from-scratch-957a17b697c5)
- [Redis Protocol Specification (RESP)](https://redis.io/docs/latest/develop/reference/protocol-spec/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Linux man pages: epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Google Sanitizers Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [Build Your Own Redis](https://build-your-own.org/redis/)

---

### 📄 Resume Snippet

**RESP-CPP** | **C++17**, **Linux `epoll`**, **POSIX Sockets**, **TCP**, **RESP Protocol**, **ASan/UBSan**, **Valkey Benchmarking**  
🔗 [**Medium Architecture Blog**](https://medium.com/@rustam98137/i-was-bored-and-had-free-time-so-i-built-redis-from-scratch-957a17b697c5) | 💻 [**GitHub Repository**](https://github.com/RustamSheoran/RESP-CPP)

- Designed and built a **single-threaded, non-blocking** Redis-compatible key-value server (`PING`/`SET`/`GET`/`DEL`) in **C++17** using **Linux `epoll`** and **POSIX sockets**, featuring a **dual-mode RESP/inline parser**, **zero-copy `std::string_view`** argument extraction, and dynamic **`EPOLLOUT` write backpressure**.
- Benchmarked against **Valkey 9.1.2** with `valkey-benchmark` under strict CPU core pinning (`taskset`, Intel i5-4430): achieved **~113K req/s** sustained over **500s endurance runs** (**within 2.7% of Valkey**), matching median latency (**p50: 0.223 ms vs 0.215 ms**).
- Hardened against Linux kernel edge cases and malformed input: resolved **`EMFILE` connection spin** via reserve file descriptors, shielded against **`SIGPIPE`** crashes, eliminated delayed ACKs with **`TCP_NODELAY`**, enforced **bounded frame allocations** and **10 MB buffer caps**, and implemented **idle connection sweeps**; verified **100% clean under ASan/UBSan**.
