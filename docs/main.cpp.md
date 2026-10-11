# Documentation: `src/main.cpp`

The [`src/main.cpp`](file:///home/rustam/Projects/resp-cpp/src/main.cpp) file is the server process entry point, establishing signal safety shields, port configuration, and running the reactor event loop.

---

### Line 1: `#include "server.h"`
Imports the `Server` class declaration to instantiate and manage the server reactor.

### Line 2: `#include <csignal>`
Imports POSIX signal handling functions and constants, specifically `signal()` and `SIGPIPE`.

### Line 3: `#include <iostream>`
Imports C++ standard I/O streams for outputting error diagnostics via `std::cerr`.

### Line 4: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 5: `constexpr int PORT = 6380;`
Declares compile-time constant `PORT` set to `6380` to prevent collisions with standard Redis instances (port 6379).

### Line 6: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 7: `int main() {`
Process entry point invoked by the operating system loader.

### Line 8: `  signal(SIGPIPE, SIG_IGN);`
Ignores the `SIGPIPE` signal (`SIG_IGN`). In Linux, attempting to write to a closed socket normally delivers `SIGPIPE`, which aborts the process by default. Ignoring this signal causes `write()` to instead return `-1` with `errno = EPIPE`, allowing safe, graceful client disconnection without crashing the server.

### Line 9: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 10: `  try {`
Begins exception handling block protecting against fatal startup errors (e.g., port binding collisions or epoll failures).

### Line 11: `    Server server(PORT);`
Constructs the `Server` instance, creating the listening socket, setting `SO_REUSEADDR`, binding, listening, configuring `O_NONBLOCK`, and initializing the `epoll` kernel instance.

### Line 12: `    server.run();`
Enters the single-threaded reactor event loop to accept connections and dispatch non-blocking I/O events.

### Line 13: `  } catch (const std::exception &e) {`
Catches standard C++ runtime exceptions derived from `std::exception`.

### Line 14: `    std::cerr << "Error: " << e.what() << "\n";`
Prints the exception's descriptive error message to standard error.

### Line 15: `    return 1;`
Terminates the application with failure status code `1`.

### Line 16: `  }`
Closes the `try/catch` block.

### Line 17: `  return 0;`
Returns success exit status code `0` to the operating system upon normal termination.

### Line 18: `}`
Closes the `main()` function body.
