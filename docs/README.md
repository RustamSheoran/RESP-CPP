# RESP-CPP Line-by-Line Code Documentation Index

Welcome to the comprehensive, line-by-line documentation for the **RESP-CPP** codebase. Each source file, header, build script, and benchmark harness in this repository has a dedicated documentation file detailing its exact lines of code, system calls, data structures, and algorithmic flow.

---

## 📚 Documentation Index

| File | Purpose | Detailed Documentation |
| :--- | :--- | :--- |
| **`src/main.cpp`** | Process entry point, `SIGPIPE` ignore shield, runtime error handling, and reactor launch | [main.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/main.cpp.md) |
| **`src/server.h`** | `Server` class interface, epoll constants, and private method declarations | [server.h.md](file:///home/rustam/Projects/resp-cpp/docs/server.h.md) |
| **`src/server.cpp`** | Socket creation, `epoll` reactor loop, `TCP_NODELAY`, Slowloris defense, and non-blocking I/O | [server.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/server.cpp.md) |
| **`src/client.h`** | `ClientState` struct defining per-connection ingress and egress backpressure buffers | [client.h.md](file:///home/rustam/Projects/resp-cpp/docs/client.h.md) |
| **`src/protocol.h`** | Protocol parser interface utilizing C++17 `std::string_view` for zero-copy parsing | [protocol.h.md](file:///home/rustam/Projects/resp-cpp/docs/protocol.h.md) |
| **`src/protocol.cpp`** | In-memory key-value database, tokenizer, and command executor (`PING`, `SET`, `GET`, `DEL`, `COMMAND`, `QUIT`) | [protocol.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/protocol.cpp.md) |
| **`src/utils.h`** | Declaration of POSIX socket utility functions | [utils.h.md](file:///home/rustam/Projects/resp-cpp/docs/utils.h.md) |
| **`src/utils.cpp`** | Implementation of `set_nonblocking()` using `fcntl(F_SETFL, O_NONBLOCK)` | [utils.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/utils.cpp.md) |
| **`Makefile`** | Build configuration, optimization targets (`release`, `asan`, `debug`), and compilation rules | [Makefile.md](file:///home/rustam/Projects/resp-cpp/docs/Makefile.md) |
| **`benchmark.py`** | Async Python load-testing suite simulating 1,000+ pipelined clients measuring QPS and latencies | [benchmark.py.md](file:///home/rustam/Projects/resp-cpp/docs/benchmark.py.md) |

---

## 🏛️ Architecture Overview

```
                      +-------------------+
                      |   Client Sockets  |
                      +---------+---------+
                                |  (TCP)
                                v
                   +------------------------+
                   |  Linux epoll Event Loop| <---+
                   |     (Server::run)      |     |
                   +-----------+------------+     |
                               |                  |
            +------------------+------------------+
            |                  |                  |
            v                  v                  v
    [accept_clients]     [handle_read]      [handle_write]
            |                  |                  |
            |            (Parse \n)               |
            |                  v                  |
            |           [handle_message]          |
            |                  |                  |
            |        (std::string_view)           |
            |                  v                  |
            |         [Protocol::execute]         |
            |                  |                  |
            |           (db[k] = val)             |
            |                  v                  |
            |         [Append write_buf] ---------+
            v                  |
    (clients[fd] created)      v (Flush / EPOLLOUT)
```

1. **Ingress:** Packets arrive via non-blocking sockets into `ClientState::read_buf`.
2. **Framing:** Delimiter slicing (`\n`) extracts discrete application lines with Slowloris bounded checks (`MAX_QUERY_LEN = 300`).
3. **Execution:** Tokens are parsed zero-copy with `std::string_view`, reading or mutating `Protocol::db` (`std::unordered_map`).
4. **Egress:** Responses are formatted into standard RESP strings and written directly. If blocked by network backpressure, remaining bytes are buffered in `ClientState::write_buf` and managed via `EPOLLOUT`.
