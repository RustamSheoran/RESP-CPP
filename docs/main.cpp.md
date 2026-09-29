# Documentation: `src/main.cpp`

The [`src/main.cpp`](file:///home/rustam/Projects/resp-cpp/src/main.cpp) file serves as the main entry point for the RESP server. It establishes signal safety shields, configures the listening port, instantiates the `Server` reactor, handles unhandled exceptions gracefully, and starts the event loop.

---

## Complete Source Code

```cpp
1: #include "server.h"
2: #include <csignal>
3: #include <iostream>
4: 
5: constexpr int PORT = 6380;
6: 
7: int main() {
8:   signal(SIGPIPE, SIG_IGN);
9: 
10:   try {
11:     Server server(PORT);
12:     server.run();
13:   } catch (const std::exception &e) {
14:     std::cerr << "Error: " << e.what() << "\n";
15:     return 1;
16:   }
17:   return 0;
18: }
```

---

## Line-by-Line Breakdown & Explanation

### Line 1: `#include "server.h"`
- **Purpose:** Includes the `Server` class header.
- **Detailed Explanation:** Exposes the constructor, destructor, and public `run()` method of the `Server` class defined in [`src/server.h`](file:///home/rustam/Projects/resp-cpp/src/server.h) to allow instantiating and controlling the server instance.

### Line 2: `#include <csignal>`
- **Purpose:** C standard signal handling library.
- **Detailed Explanation:** Provides POSIX signal management symbols, specifically the `signal()` function and the `SIGPIPE` signal constant.

### Line 3: `#include <iostream>`
- **Purpose:** Standard input/output stream library.
- **Detailed Explanation:** Provides access to `std::cerr` for formatted error reporting to standard error when fatal runtime exceptions occur.

### Line 4: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 5: `constexpr int PORT = 6380;`
- **Purpose:** Compile-time constant definition for the listening TCP port.
- **Detailed Explanation:**
  - `constexpr`: Evaluated at compile-time by the compiler, avoiding runtime overhead and guaranteeing immutability.
  - `PORT = 6380`: Chooses port `6380` instead of Redis's default `6379`, avoiding port collision with existing system Redis daemons during local development and benchmarking.

### Line 6: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 7: `int main() {`
- **Purpose:** Standard application entry point.
- **Detailed Explanation:** The starting point of execution when the binary `./build/resp-server` is launched by the OS loader. Returns an integer exit code to the operating system shell upon termination.

### Line 8: `  signal(SIGPIPE, SIG_IGN);`
- **Purpose:** Ignore the `SIGPIPE` signal (`SIG_IGN`).
- **Detailed Explanation:**
  - **The Problem:** In Unix/Linux, if a client abruptly closes its TCP connection (or sends a TCP RST packet) and the server attempts to execute `write()` or `send()` to that socket, the Linux kernel generates a `SIGPIPE` signal directed at the process.
  - **Default Behavior:** By default, the operating system's default handler for `SIGPIPE` terminates the process immediately without stack unwinding or error messages.
  - **The Solution:** Calling `signal(SIGPIPE, SIG_IGN)` tells the kernel to discard `SIGPIPE`. Consequently, `write()` simply fails and returns `-1` with `errno` set to `EPIPE` (Broken pipe). The server's I/O logic inspects this return code, closes the dead client socket gracefully, and continues servicing other clients without crashing.

### Line 9: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 10: `  try {`
- **Purpose:** Exception handling block initiation.
- **Detailed Explanation:** Encloses the server initialization and execution loop in a standard C++ `try` block to catch and report critical runtime errors (e.g., port already bound, epoll creation failure).

### Line 11: `    Server server(PORT);`
- **Purpose:** Construct the `Server` object instance.
- **Detailed Explanation:**
  - Invokes the `Server::Server(int port)` constructor.
  - Creates the TCP stream socket (`socket()`), enables address reuse (`SO_REUSEADDR`), binds the socket to `0.0.0.0:6380` (`bind()`), marks it as listening (`listen()`), configures non-blocking mode via `set_nonblocking()`, creates the `epoll` kernel instance (`epoll_create1()`), and registers the server socket with `EPOLLIN`.

### Line 12: `    server.run();`
- **Purpose:** Start the event-driven reactor loop.
- **Detailed Explanation:** Calls `Server::run()`, which enters an infinite `epoll_wait()` loop. The single thread waits for I/O events on any registered file descriptor, dispatching accept, read, or write operations as readiness notifications arrive from the kernel.

### Line 13: `  } catch (const std::exception &e) {`
- **Purpose:** Catch unhandled exceptions deriving from `std::exception`.
- **Detailed Explanation:** Intercepts runtime failures thrown during construction or operation (such as `std::runtime_error("Failed to bind")` when port 6380 is already in use by another process).

### Line 14: `    std::cerr << "Error: " << e.what() << "\n";`
- **Purpose:** Print the fatal error message.
- **Detailed Explanation:** Writes the exception's descriptive message (`e.what()`) directly to standard error (`std::cerr`), informing the operator of why the server could not start.

### Line 15: `    return 1;`
- **Purpose:** Failure exit status.
- **Detailed Explanation:** Exits the process with status code `1`, indicating an abnormal termination to scripts, shells, or process supervisors (e.g. systemd).

### Line 16: `  }`
- **Purpose:** Closes the `catch` block.

### Line 17: `  return 0;`
- **Purpose:** Successful program exit status.
- **Detailed Explanation:** If `server.run()` terminates normally (e.g., via a graceful shutdown mechanism), `main` returns `0` indicating successful execution.

### Line 18: `}`
- **Purpose:** Closes the `main` function body.
