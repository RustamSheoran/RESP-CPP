# Documentation: `src/server.h`

The [`src/server.h`](file:///home/rustam/Projects/resp-cpp/src/server.h) header file declares the `Server` class, which manages the socket lifecycle, Linux `epoll` reactor event loop, client connection states, and non-blocking I/O event dispatching.

---

## Complete Source Code

```cpp
1: #ifndef SERVER_H
2: #define SERVER_H
3: 
4: #include <unordered_map>
5: #include "client.h"
6: 
7: class Server {
8: public:
9:     Server(int port);
10:     ~Server();
11:     void run();
12: 
13: private:
14:     void accept_clients();
15:     void handle_read(int client_fd);
16:     void handle_write(int client_fd);
17:     void handle_message(int client_fd, const std::string &message);
18:     void close_client(int client_fd);
19: 
20:     int port;
21:     int server_fd;
22:     int epfd;
23:     std::unordered_map<int, ClientState> clients;
24:     
25:     static constexpr int MAX_EVENTS = 64;
26: };
27: 
28: #endif
```

---

## Line-by-Line Breakdown & Explanation

### Line 1: `#ifndef SERVER_H`
- **Purpose:** Include guard conditional.
- **Detailed Explanation:** Prevents multiple inclusions of the `Server` class definition in the same compilation pass, which would otherwise trigger class redefinition errors.

### Line 2: `#define SERVER_H`
- **Purpose:** Macro definition for the include guard.
- **Detailed Explanation:** Marks `SERVER_H` as defined.

### Line 3: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 4: `#include <unordered_map>`
- **Purpose:** C++ Standard Library hash map container.
- **Detailed Explanation:** Provides `std::unordered_map`, used to maintain an in-memory mapping from an open file descriptor integer (`int client_fd`) to its corresponding [`ClientState`](file:///home/rustam/Projects/resp-cpp/src/client.h) object with $O(1)$ amortized average lookup time.

### Line 5: `#include "client.h"`
- **Purpose:** Include client connection state definition.
- **Detailed Explanation:** Imports `ClientState`, defining the struct that holds per-connection input/output buffers and descriptor identifiers.

### Line 6: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 7: `class Server {`
- **Purpose:** Class declaration opening.
- **Detailed Explanation:** Declares the `Server` encapsulation boundary responsible for network socket management, reactor event polling, and client session lifecycles.

### Line 8: `public:`
- **Purpose:** Public access specifier.
- **Detailed Explanation:** Exposes methods that can be called by external modules (such as `main.cpp`).

### Line 9: `    Server(int port);`
- **Purpose:** Constructor declaration.
- **Detailed Explanation:** Accepts the integer TCP port number on which the server should listen (e.g. 6380). Initializes listening sockets and the `epoll` kernel instance.

### Line 10: `    ~Server();`
- **Purpose:** Destructor declaration.
- **Detailed Explanation:** Responsible for RAII cleanup, ensuring that the listening socket (`server_fd`) and the epoll file descriptor (`epfd`) are cleanly closed if the `Server` instance goes out of scope.

### Line 11: `    void run();`
- **Purpose:** Core event loop method.
- **Detailed Explanation:** Enters the infinite `epoll_wait` event loop, blocking on kernel readiness events and dispatching them to `accept_clients`, `handle_read`, or `handle_write`.

### Line 12: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 13: `private:`
- **Purpose:** Private access specifier.
- **Detailed Explanation:** Restricts internal helper methods and state variables so they cannot be accessed or manipulated directly from outside the class.

### Line 14: `    void accept_clients();`
- **Purpose:** Connection acceptor method declaration.
- **Detailed Explanation:** Called when the listening `server_fd` has an `EPOLLIN` event. Executes a non-blocking `accept()` loop to accept all queued incoming connections from the kernel backlog.

### Line 15: `    void handle_read(int client_fd);`
- **Purpose:** Read event handler declaration.
- **Detailed Explanation:** Called when an established client socket has data available in its kernel receive buffer (`EPOLLIN`). Reads data into `ClientState::read_buf`, checks Slowloris DoS bounds, parses delimiter-framed lines, and dispatches messages.

### Line 16: `    void handle_write(int client_fd);`
- **Purpose:** Write readiness event handler declaration.
- **Detailed Explanation:** Called when a client socket becomes ready to accept outgoing bytes (`EPOLLOUT`). Flushes pending bytes from `ClientState::write_buf` to the network, and deregisters `EPOLLOUT` once the buffer is emptied.

### Line 17: `    void handle_message(int client_fd, const std::string &message);`
- **Purpose:** Single message parser and dispatcher.
- **Detailed Explanation:** Processes a single delimiter-extracted command line, trims whitespace, passes the zero-copy view to `Protocol::execute_command()`, and enqueues the resulting RESP response into the client's write buffer.

### Line 18: `    void close_client(int client_fd);`
- **Purpose:** Graceful client teardown method.
- **Detailed Explanation:** Removes the client descriptor from the epoll set (`EPOLL_CTL_DEL`), closes the OS socket descriptor (`close()`), and erases the client's state from the `clients` map.

### Line 19: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 20: `    int port;`
- **Purpose:** Server listening port storage.
- **Detailed Explanation:** Stores the TCP port number passed into the constructor (e.g., 6380).

### Line 21: `    int server_fd;`
- **Purpose:** Master listening socket file descriptor.
- **Detailed Explanation:** Stores the integer socket file descriptor created by `socket(AF_INET, SOCK_STREAM, 0)`.

### Line 22: `    int epfd;`
- **Purpose:** Epoll instance file descriptor.
- **Detailed Explanation:** Stores the kernel event polling descriptor created by `epoll_create1(0)`.

### Line 23: `    std::unordered_map<int, ClientState> clients;`
- **Purpose:** Active client connection registry.
- **Detailed Explanation:** Fast hash map mapping an active client socket file descriptor (`int client_fd`) to its corresponding [`ClientState`](file:///home/rustam/Projects/resp-cpp/src/client.h) object holding connection buffers.

### Line 24: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 25: `    static constexpr int MAX_EVENTS = 64;`
- **Purpose:** Maximum batch size for `epoll_wait`.
- **Detailed Explanation:**
  - `static constexpr`: Class-level compile-time constant.
  - Specifies that each call to `epoll_wait` can retrieve up to 64 ready I/O events in a single system call batch, balancing throughput with low memory footprint on the stack.

### Line 26: `};`
- **Purpose:** Class declaration closing delimiter.

### Line 27: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 28: `#endif`
- **Purpose:** Include guard terminator.
- **Detailed Explanation:** Closes `#ifndef SERVER_H`.
