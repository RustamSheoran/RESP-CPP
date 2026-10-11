# Documentation: `src/server.h`

The [`src/server.h`](file:///home/rustam/Projects/resp-cpp/src/server.h) header file declares the `Server` class, epoll constants, and configuration parameters.

---

### Line 1: `#ifndef SERVER_H`
Header include guard preventing multiple inclusions of `Server` class declarations within the same compilation unit.

### Line 2: `#define SERVER_H`
Defines `SERVER_H` to enforce include guard protection.

### Line 3: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 4: `#include <unordered_map>`
Imports `std::unordered_map` for constant-time $O(1)$ client descriptor lookups.

### Line 5: `#include "client.h"`
Imports `ClientState` struct holding connection descriptors, buffers, and timestamps.

### Line 6: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 7: `class Server {`
Declares the `Server` class encapsulating network sockets and the epoll reactor.

### Line 8: `public:`
Public access specifier exposing external lifecycle methods.

### Line 9: `    Server(int port);`
Constructor accepting the listening TCP port (e.g. 6380).

### Line 10: `    ~Server();`
Destructor ensuring clean release of listening sockets, epoll descriptors, and spare descriptors.

### Line 11: `    void run();`
Method that initiates and drives the single-threaded reactor event loop.

### Line 12: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 13: `private:`
Private access specifier hiding internal reactor methods and state.

### Line 14: `    void accept_clients();`
Non-blocking accept loop method with `EMFILE` connection exhaustion protection.

### Line 15: `    void handle_read(int client_fd);`
Ingress read handler reading bytes and dispatching streaming requests.

### Line 16: `    void handle_write(int client_fd);`
Egress handler flushing write buffers under dynamic `EPOLLOUT` notifications.

### Line 17: `    void close_client(int client_fd);`
Connection teardown method closing sockets and deallocating client state.

### Line 18: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 19: `    int port;`
Stores the configured listening port number.

### Line 20: `    int server_fd;`
Stores the master listening socket file descriptor.

### Line 21: `    int epfd;`
Stores the Linux `epoll` kernel instance descriptor.

### Line 22: `    int idle_fd;`
Stores a reserved file descriptor (opened to `/dev/null`) used to drain pending connections during `EMFILE` descriptor exhaustion, preventing level-triggered 100% CPU spin.

### Line 23: `    std::unordered_map<int, ClientState> clients;`
Hash map indexing active client sessions by their socket file descriptor.

### Line 24: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 25: `    static constexpr int MAX_EVENTS = 64;`
Maximum event batch size returned by each call to `epoll_wait()`.

### Line 26: `    static constexpr size_t MAX_WRITE_BUF_LEN = 10 * 1024 * 1024;`
10 MB backpressure write buffer ceiling to prevent memory exhaustion from slow/unresponsive readers.

### Line 27: `    static constexpr int IDLE_TIMEOUT_SECONDS = 60;`
60-second connection inactivity timeout protecting against idle connection Slowloris exhaustion.

### Line 28: `};`
Closes `Server` class declaration.

### Line 29: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 30: `#endif`
Terminates the `#ifndef SERVER_H` include guard.
