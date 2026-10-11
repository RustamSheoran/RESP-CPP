# RESP-CPP Line-by-Line Code Documentation Index

Welcome to the comprehensive, line-by-line documentation for the **RESP-CPP** codebase. Each source file, header, and build script in this repository has a dedicated documentation file detailing its exact lines of code, system calls, data structures, and algorithmic flow.

---

## 📚 Documentation Index

| File | Purpose | Detailed Documentation |
| :--- | :--- | :--- |
| **`src/main.cpp`** | Process entry point, `SIGPIPE` ignore shield, runtime error handling, and reactor launch | [main.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/main.cpp.md) |
| **`src/server.h`** | `Server` class interface, epoll constants, `EMFILE` idle descriptor, and timeout parameters | [server.h.md](file:///home/rustam/Projects/resp-cpp/docs/server.h.md) |
| **`src/server.cpp`** | Socket creation, `epoll` reactor loop, `TCP_NODELAY`, `EMFILE` spin cure, idle sweep, dynamic `EPOLLOUT`, and `QUIT` teardown | [server.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/server.cpp.md) |
| **`src/client.h`** | `ClientState` struct defining per-connection ingress, egress backpressure buffer, teardown flag, and timestamp | [client.h.md](file:///home/rustam/Projects/resp-cpp/docs/client.h.md) |
| **`src/protocol.h`** | Protocol parser interface, `ParseResult` enum, and streaming `parse_and_execute()` function prototype | [protocol.h.md](file:///home/rustam/Projects/resp-cpp/docs/protocol.h.md) |
| **`src/protocol.cpp`** | Dual-mode parser (RESP Multibulk Arrays `*N\r\n` + quote-aware inline commands), hash map database, and command dispatch | [protocol.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/protocol.cpp.md) |
| **`src/utils.h`** | Declaration of POSIX socket utility functions | [utils.h.md](file:///home/rustam/Projects/resp-cpp/docs/utils.h.md) |
| **`src/utils.cpp`** | Implementation of `set_nonblocking()` using `fcntl(F_SETFL, O_NONBLOCK)` | [utils.cpp.md](file:///home/rustam/Projects/resp-cpp/docs/utils.cpp.md) |
| **`Makefile`** | Multi-target build configuration (`release`, `asan`, `debug`) and compilation rules | [Makefile.md](file:///home/rustam/Projects/resp-cpp/docs/Makefile.md) |

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
            |     (RESP Array OR Inline \n)       |
            |                  v                  |
            |      [parse_and_execute]            |
            |                  |                  |
            |         [execute_tokens]            |
            |                  |                  |
            |           (db[k] = val)             |
            |                  v                  |
            |         [Append write_buf] ---------+
            v                  |
    (clients[fd] created)      v (Flush / EPOLLOUT)
```

1. **Ingress:** Packets arrive via non-blocking sockets into `ClientState::read_buf`.
2. **Dual-Mode Framing:** 
   - **Official RESP Arrays (`*N\r\n...`):** Parsed binary-safely with incomplete payload buffering (`NEED_MORE_DATA`).
   - **Inline Commands (`\n`):** Parsed with a quote-aware tokenizer supporting quoted keys and values (`"hello world"`).
3. **Execution:** Dispatches commands (`PING`, `SET`, `GET`, `DEL`, `COMMAND`, `INFO`, `QUIT`, `HELLO`), querying or mutating `Protocol::db` (`std::unordered_map`).
4. **Egress & Backpressure:** Responses are formatted into standard RESP strings and written directly. If blocked by network backpressure, remaining bytes are buffered in `ClientState::write_buf` (capped at 10 MB) and flushed via `EPOLLOUT`.
5. **Teardown & Timeouts:** `QUIT` flushes `+OK\r\n` before closing the socket. Connections idle for > 60 seconds are automatically closed. `EMFILE` errors are safely handled via a reserved spare descriptor.
