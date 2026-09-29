# Documentation: `src/client.h`

The [`src/client.h`](file:///home/rustam/Projects/resp-cpp/src/client.h) header file defines the `ClientState` struct, which encapsulates all persistent, per-connection data and I/O buffers required by the single-threaded `epoll` reactor.

---

## Complete Source Code

```cpp
1: #ifndef CLIENT_H
2: #define CLIENT_H
3: 
4: #include <string>
5: 
6: struct ClientState {
7:     int fd;
8:     std::string read_buf;
9:     std::string write_buf;
10: };
11: 
12: #endif
```

---

## Line-by-Line Breakdown & Explanation

### Line 1: `#ifndef CLIENT_H`
- **Purpose:** Header guard conditional.
- **Detailed Explanation:** Checks whether `CLIENT_H` has been previously defined in the current translation unit. If already defined, the preprocessor skips the file down to `#endif`, preventing duplicate type definitions of `struct ClientState`.

### Line 2: `#define CLIENT_H`
- **Purpose:** Macro definition for the header guard.
- **Detailed Explanation:** Defines `CLIENT_H` so subsequent inclusions in the same compilation pass will be skipped.

### Line 3: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 4: `#include <string>`
- **Purpose:** Standard C++ string library header.
- **Detailed Explanation:** Imports `std::string`, providing dynamically resizable contiguous memory buffers used for `read_buf` and `write_buf`.

### Line 5: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 6: `struct ClientState {`
- **Purpose:** Structure definition opening.
- **Detailed Explanation:** In C++, `struct` members are `public` by default. `ClientState` acts as a Plain Old Data (POD) / lightweight container holding the socket descriptor and both the ingress and egress byte streams for an individual client.

### Line 7: `    int fd;`
- **Purpose:** Client socket descriptor.
- **Detailed Explanation:** Holds the integer file descriptor returned by the Linux `accept()` system call. This descriptor uniquely identifies the TCP connection in the kernel and is used as the key in the server's client registry (`std::unordered_map<int, ClientState> clients`).

### Line 8: `    std::string read_buf;`
- **Purpose:** Ingress byte accumulation buffer.
- **Detailed Explanation:**
  - TCP is a stream-oriented protocol, meaning packets can arrive fragmented (e.g. half a command in one packet, the rest in another) or coalesced (multiple commands in one packet).
  - When `read()` is called on a non-blocking socket, all incoming bytes are appended to `read_buf`.
  - The reactor scans `read_buf` for the delimiter `\n` to isolate discrete application messages. Once a full message is parsed, it is sliced and removed from `read_buf`.

### Line 9: `    std::string write_buf;`
- **Purpose:** Egress response buffering (backpressure handling).
- **Detailed Explanation:**
  - When the server generates RESP responses (e.g., `+OK\r\n`), it attempts an immediate non-blocking `write()`.
  - If the socket's kernel transmit buffer is full or cannot absorb the entire response in a single non-blocking call, the unsent remainder is stored in `write_buf`.
  - The server then registers `EPOLLOUT` with the kernel. Once the kernel's transmit buffer frees up space, `epoll` wakes up `handle_write()`, which drains `write_buf`.
  - This prevents socket blocking, dropped responses, and CPU busy-spinning.

### Line 10: `};`
- **Purpose:** Structure closing delimiter.
- **Detailed Explanation:** Ends the declaration of `ClientState`.

### Line 11: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 12: `#endif`
- **Purpose:** Terminating directive of the `#ifndef CLIENT_H` header guard.
