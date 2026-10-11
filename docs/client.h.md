# Documentation: `src/client.h`

The [`src/client.h`](file:///home/rustam/Projects/resp-cpp/src/client.h) header file defines the `ClientState` struct, which encapsulates all per-connection state, buffers, and activity tracking.

---

### Line 1: `#ifndef CLIENT_H`
Header include guard checking if the `CLIENT_H` macro has not yet been defined in the current translation unit.

### Line 2: `#define CLIENT_H`
Defines `CLIENT_H` to prevent duplicate type definitions of `struct ClientState` if this header is included multiple times.

### Line 3: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 4: `#include <chrono>`
Imports standard C++ monotonic time facilities (`std::chrono::steady_clock`), utilized to record client activity timestamps for connection idle timeouts.

### Line 5: `#include <string>`
Imports `std::string` to provide dynamically resizable byte buffers for ingress and egress streams.

### Line 6: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 7: `struct ClientState {`
Declares the `ClientState` structure representing an active client connection.

### Line 8: `    int fd;`
Stores the socket file descriptor returned by the kernel's `accept()` system call, uniquely identifying the client socket.

### Line 9: `    std::string read_buf;`
Ingress accumulation buffer storing raw bytes received from non-blocking `read()` calls across fragmented network packets until complete commands are framed.

### Line 10: `    std::string write_buf;`
Egress backpressure buffer accumulating serialized RESP responses that cannot be completely flushed in a single non-blocking `write()`, dynamically monitored by `EPOLLOUT`.

### Line 11: `    bool should_close;`
Graceful disconnection flag set to `true` when the client sends `QUIT`. Directs the server to flush any pending data in `write_buf` before closing the socket.

### Line 12: `    std::chrono::steady_clock::time_point last_active;`
Monotonic timestamp recording the last time the client connected or sent data, checked by the reactor's idle connection timeout sweep.

### Line 13: `};`
Closing delimiter of the `ClientState` structure.

### Line 14: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 15: `#endif`
Terminates the `#ifndef CLIENT_H` header guard.
