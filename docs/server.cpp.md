# Documentation: `src/server.cpp`

The [`src/server.cpp`](file:///home/rustam/Projects/resp-cpp/src/server.cpp) file implements the networking reactor core, connection accept loop, 16 KB non-blocking I/O buffers, pre-allocated client streams, batch cursor parsing, `EMFILE` spin protection, rate-limited idle connection sweeps, backpressure limits, and graceful `QUIT` teardown.

---

### Line 1: `#include "server.h"`
Imports the `Server` class declarations and configuration constants.

### Line 2: `#include "utils.h"`
Imports `set_nonblocking()` socket configuration utility.

### Line 3: `#include "protocol.h"`
Imports `Protocol::parse_and_execute()` protocol parser interface.

### Line 4: `#include <cstdio>`
Imports C standard I/O library for console status output (`printf`).

### Line 5: `#include <cstring>`
Imports memory management functions such as `memset`.

### Line 6: `#include <vector>`
Imports `std::vector` for collecting file descriptors during idle sweeps.

### Line 7: `#include <chrono>`
Imports `std::chrono` monotonic clock utilities for activity timestamping and sweep rate-limiting.

### Line 8: `#include <fcntl.h>`
Imports POSIX file control declarations for `open()` and `O_RDONLY`.

### Line 9: `#include <netinet/in.h>`
Imports Internet socket address structure `sockaddr_in` and `htons()`.

### Line 10: `#include <netinet/tcp.h>`
Imports TCP options, specifically `TCP_NODELAY`.

### Line 11: `#include <stdexcept>`
Imports exception classes including `std::runtime_error`.

### Line 12: `#include <sys/epoll.h>`
Imports Linux event polling system calls: `epoll_create1`, `epoll_ctl`, `epoll_wait`, `EPOLLIN`, `EPOLLOUT`.

### Line 13: `#include <sys/socket.h>`
Imports POSIX socket calls: `socket`, `bind`, `listen`, `accept`, `setsockopt`, `SO_REUSEADDR`.

### Line 14: `#include <unistd.h>`
Imports POSIX system calls: `read`, `write`, `close`.

### Line 15: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 16: `Server::Server(int port) : port(port), server_fd(-1), epfd(-1), idle_fd(-1), last_sweep(std::chrono::steady_clock::now()) {`
Constructor initializing listening port, initializing descriptor members to `-1`, and seeding `last_sweep` with the current monotonic timestamp.

### Line 17: `  server_fd = socket(AF_INET, SOCK_STREAM, 0);`
Creates an IPv4 TCP byte-stream socket.

### Line 18: `  if (server_fd < 0) {`
### Line 19: `    throw std::runtime_error("Failed to create socket");`
### Line 20: `  }`
Throws runtime exception if socket creation fails.

### Line 21: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 22: `  int opt = 1;`
### Line 23: `  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));`
Enables `SO_REUSEADDR` to prevent address-in-use errors during rapid restarts.

### Line 24: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 25: `  sockaddr_in addr{};`
### Line 26: `  addr.sin_family = AF_INET;`
### Line 27: `  addr.sin_addr.s_addr = INADDR_ANY;`
### Line 28: `  addr.sin_port = htons(port);`
Configures socket address to bind to all network interfaces on the specified port.

### Line 29: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 30: `  if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {`
### Line 31: `    throw std::runtime_error("Failed to bind");`
### Line 32: `  }`
Binds the socket to the port, throwing on failure.

### Line 33: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 34: `  if (listen(server_fd, 128) < 0) {`
### Line 35: `    throw std::runtime_error("Failed to listen");`
### Line 36: `  }`
Marks socket as passive with a listen backlog of 128 connections.

### Line 37: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 38: `  set_nonblocking(server_fd);`
Configures the listening socket for non-blocking I/O (`O_NONBLOCK`).

### Line 39: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 40: `  epfd = epoll_create1(0);`
### Line 41: `  if (epfd < 0) {`
### Line 42: `    throw std::runtime_error("Failed to create epoll");`
### Line 43: `  }`
Creates the Linux `epoll` instance, throwing on failure.

### Line 44: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 45: `  idle_fd = open("/dev/null", O_RDONLY);`
Opens reserved spare file descriptor to `/dev/null` for `EMFILE` spin prevention.

### Line 46: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 47: `  epoll_event ev{};`
### Line 48: `  ev.events = EPOLLIN;`
### Line 49: `  ev.data.fd = server_fd;`
### Line 50: `  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);`
Registers listening socket with epoll for incoming connection events (`EPOLLIN`).

### Line 51: `}`
Closes `Server` constructor.

### Line 52: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 53: `Server::~Server() {`
Destructor performing RAII cleanup of system resources.

### Line 54: `  if (server_fd >= 0)`
### Line 55: `    close(server_fd);`
Closes master listening socket if open.

### Line 56: `  if (epfd >= 0)`
### Line 57: `    close(epfd);`
Closes epoll file descriptor if open.

### Line 58: `  if (idle_fd >= 0)`
### Line 59: `    close(idle_fd);`
Closes spare file descriptor if open.

### Line 60: `}`
Closes destructor.

### Line 61: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 62: `void Server::run() {`
Core reactor event loop method.

### Line 63: `  epoll_event events[MAX_EVENTS];`
Allocates stack array receiving ready I/O events.

### Line 64: `  printf("listening on port %d\n", port);`
Prints startup listening confirmation.

### Line 65: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 66: `  while (true) {`
Enters infinite event loop.

### Line 67: `    int n = epoll_wait(epfd, events, MAX_EVENTS, 1000);`
Waits for ready events with a 1,000 ms timeout to allow periodic idle connection sweeps.

### Line 68: `    for (int i = 0; i < n; i++) {`
Iterates over ready events.

### Line 69: `      int fd = events[i].data.fd;`
Extracts descriptor associated with ready event.

### Line 70: `      if (fd == server_fd) {`
### Line 71: `        accept_clients();`
Invokes accept handler if listening socket is readable.

### Line 72: `      } else {`
### Line 73: `        if (events[i].events & EPOLLIN) {`
### Line 74: `          handle_read(fd);`
### Line 75: `        }`
Invokes read handler if client socket has incoming bytes.

### Line 76: `        if (events[i].events & EPOLLOUT) {`
### Line 77: `          handle_write(fd);`
### Line 78: `        }`
Invokes write handler if client socket is ready for outgoing bytes.

### Line 79: `      }`
### Line 80: `    }`
Closes event iteration loop.

### Line 81: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 82: `    auto now = std::chrono::steady_clock::now();`
Captures current monotonic timestamp for timeout and sweep evaluations.

### Line 83: `    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_sweep).count() >= 1) {`
Rate-limits idle connection sweeps to run at most once per second rather than on every single reactor spin.

### Line 84: `      last_sweep = now;`
Updates timestamp of most recent idle connection sweep.

### Line 85: `      std::vector<int> to_close;`
Initializes list of idle connection descriptors to close.

### Line 86: `      for (const auto &[cfd, state] : clients) {`
### Line 87: `        if (std::chrono::duration_cast<std::chrono::seconds>(now - state.last_active).count() > IDLE_TIMEOUT_SECONDS) {`
### Line 88: `          to_close.push_back(cfd);`
### Line 89: `        }`
### Line 90: `      }`
Identifies client connections idle for longer than `IDLE_TIMEOUT_SECONDS` (60s).

### Line 91: `      for (int cfd : to_close) {`
### Line 92: `        close_client(cfd);`
### Line 93: `      }`
Gracefully disconnects expired idle connections.

### Line 94: `    }`
Closes rate-limited sweep conditional block.

### Line 95: `  }`
Closes `while` loop.

### Line 96: `}`
Closes `run()`.

### Line 97: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 98: `void Server::accept_clients() {`
Handles incoming client connection requests.

### Line 99: `  while (true) {`
Loops accepting all pending connections in backlog queue.

### Line 100: `    int client_fd = accept(server_fd, nullptr, nullptr);`
Accepts incoming connection request.

### Line 101: `    if (client_fd < 0) {`
### Line 102: `      if (errno == EMFILE || errno == ENFILE) {`
### Line 103: `        if (idle_fd >= 0) {`
### Line 104: `          close(idle_fd);`
### Line 105: `          int temp_fd = accept(server_fd, nullptr, nullptr);`
### Line 106: `          if (temp_fd >= 0) close(temp_fd);`
### Line 107: `          idle_fd = open("/dev/null", O_RDONLY);`
### Line 108: `        }`
### Line 109: `      }`
### Line 110: `      break;`
### Line 111: `    }`
Drains excess socket when `EMFILE`/`ENFILE` occurs to prevent level-triggered 100% CPU spin. Exits loop when backlog is empty.

### Line 112: `    ClientState state{client_fd, "", "", false, std::chrono::steady_clock::now()};`
Initializes new client state with current timestamp.

### Line 113: `    state.read_buf.reserve(16384);`
### Line 114: `    state.write_buf.reserve(16384);`
Pre-allocates 16 KB capacity on ingress and egress buffers to avoid memory reallocations during benchmark bursts.

### Line 115: `    clients[client_fd] = std::move(state);`
Moves pre-allocated state into client map.

### Line 116: `    set_nonblocking(client_fd);`
Configures accepted socket for non-blocking I/O.

### Line 117: `    int nodelay = 1;`
### Line 118: `    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));`
Disables Nagle's algorithm (`TCP_NODELAY`) to eliminate 40ms delayed-ACK latency stalls.

### Line 119: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 120: `    epoll_event cev{};`
### Line 121: `    cev.events = EPOLLIN;`
### Line 122: `    cev.data.fd = client_fd;`
### Line 123: `    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);`
Registers client socket descriptor with epoll for read readiness (`EPOLLIN`).

### Line 124: `#ifdef VERBOSE`
### Line 125: `    printf("client %d connected\n", client_fd);`
### Line 126: `#endif`
Conditionally logs connection under verbose debugging builds.

### Line 127: `  }`
Closes accept loop.

### Line 128: `}`
Closes `accept_clients()`.

### Line 129: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 130: `void Server::handle_read(int fd) {`
Ingress read handler reading data and processing requests.

### Line 131: `  char buf[16384];`
Allocates a 16 KB stack buffer to ingest large pipelined batches in fewer system calls.

### Line 132: `  ssize_t count = read(fd, buf, sizeof(buf));`
Performs non-blocking read into stack buffer.

### Line 133: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 134: `  if (count <= 0) {`
### Line 135: `    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {`
### Line 136: `      return;`
### Line 137: `    }`
### Line 138: `    close_client(fd);`
### Line 139: `    return;`
### Line 140: `  }`
Returns on `EAGAIN`; closes client on EOF (`count == 0`) or fatal socket read error.

### Line 141: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 142: `  auto &state = clients[fd];`
Retrieves reference to client state.

### Line 143: `  state.last_active = std::chrono::steady_clock::now();`
Updates client activity timestamp.

### Line 144: `  state.read_buf.append(buf, count);`
Appends incoming bytes to client ingress buffer.

### Line 145: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 146: `  if (state.read_buf.size() > MAX_READ_BUF_LEN) {`
### Line 147: `    close_client(fd);`
### Line 148: `    return;`
### Line 149: `  }`
Enforces 10 MB ingress buffer ceiling, dropping abusive or malformed connections that send boundless data without valid command framing.

### Line 150: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 151: `  size_t cursor = 0;`
Initializes read offset cursor for batch parsing.

### Line 152: `  while (cursor < state.read_buf.size()) {`
Loops advancing `cursor` through `read_buf` without shifting memory per command.

### Line 153: `    bool should_close = false;`
### Line 154: `    auto res = Protocol::parse_and_execute(state.read_buf, cursor, state.write_buf, should_close);`
Parses and executes command, directly appending responses into `state.write_buf`.

### Line 155: `    if (res == Protocol::ParseResult::NEED_MORE_DATA) {`
### Line 156: `      break;`
### Line 157: `    }`
Breaks parsing loop if partial command frame is encountered.

### Line 158: `    if (should_close) {`
### Line 159: `      state.should_close = true;`
### Line 160: `    }`
Sets teardown flag if `QUIT` or a fatal protocol violation was executed.

### Line 161: `    if (state.write_buf.size() > MAX_WRITE_BUF_LEN) {`
### Line 162: `      close_client(fd);`
### Line 163: `      return;`
### Line 164: `    }`
Enforces 10 MB write buffer backpressure cap.

### Line 165: `  }`
Closes streaming parse loop.

### Line 166: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 167: `  if (cursor > 0) {`
### Line 168: `    state.read_buf.erase(0, cursor);`
### Line 169: `  }`
Performs a single memory erase at the end of the batch, eliminating $O(K^2)$ memory copying.

### Line 170: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 171: `  if (!state.write_buf.empty()) {`
### Line 172: `    ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());`
### Line 173: `    if (written > 0) {`
### Line 174: `      state.write_buf.erase(0, written);`
### Line 175: `    } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {`
### Line 176: `      close_client(fd);`
### Line 177: `      return;`
### Line 178: `    }`
### Line 179: `  }`
Attempts immediate non-blocking flush to network.

### Line 180: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 181: `  if (state.should_close && state.write_buf.empty()) {`
### Line 182: `    close_client(fd);`
### Line 183: `    return;`
### Line 184: `  }`
Closes client socket immediately if `QUIT` or protocol termination was requested and output buffer has drained.

### Line 185: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 186: `  if (!state.write_buf.empty()) {`
### Line 187: `    epoll_event cev{};`
### Line 188: `    cev.events = EPOLLIN | EPOLLOUT;`
### Line 189: `    cev.data.fd = fd;`
### Line 190: `    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);`
### Line 191: `  }`
Registers `EPOLLOUT` to resume transmission once socket becomes writable.

### Line 192: `}`
Closes `handle_read()`.

### Line 193: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 194: `void Server::handle_write(int fd) {`
Egress flush handler invoked on `EPOLLOUT` events.

### Line 195: `  if (clients.find(fd) == clients.end())`
### Line 196: `    return;`
Safety check verifying descriptor exists.

### Line 197: `  auto &state = clients[fd];`
Retrieves client state.

### Line 198: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 199: `  ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());`
### Line 200: `  if (written > 0) {`
### Line 201: `    state.write_buf.erase(0, written);`
### Line 202: `  } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {`
### Line 203: `    close_client(fd);`
### Line 204: `    return;`
### Line 205: `  }`
Flushes pending bytes and removes sent data from `write_buf`.

### Line 206: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 207: `  if (state.write_buf.empty()) {`
### Line 208: `    if (state.should_close) {`
### Line 209: `      close_client(fd);`
### Line 210: `      return;`
### Line 211: `    }`
### Line 212: `    epoll_event cev{};`
### Line 213: `    cev.events = EPOLLIN;`
### Line 214: `    cev.data.fd = fd;`
### Line 215: `    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);`
### Line 216: `  }`
When write buffer is fully drained: closes connection if `should_close` was requested, or modifies epoll registration back to `EPOLLIN` only to avoid 100% CPU busy-spinning on writable sockets.

### Line 217: `}`
Closes `handle_write()`.

### Line 218: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 219: `void Server::close_client(int fd) {`
Graceful client teardown and resource release handler.

### Line 220: `  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);`
Deregisters descriptor from epoll watch set.

### Line 221: `  close(fd);`
Closes underlying OS socket descriptor.

### Line 222: `  clients.erase(fd);`
Removes client state from registry map.

### Line 223: `#ifdef VERBOSE`
### Line 224: `  printf("client %d disconnected\n", fd);`
### Line 225: `#endif`
Conditionally logs disconnection under verbose debugging builds.

### Line 226: `}`
Closes `close_client()`.
