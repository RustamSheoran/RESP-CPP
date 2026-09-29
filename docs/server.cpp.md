# Documentation: `src/server.cpp`

The [`src/server.cpp`](file:///home/rustam/Projects/resp-cpp/src/server.cpp) file implements the networking engine of the RESP server: socket configuration, the single-threaded Linux `epoll` reactor loop, connection acceptance, non-blocking reading, delimiter framing, Slowloris DoS protection, dynamic write backpressure (`EPOLLOUT`), and client connection teardown.

---

## Complete Source Code

```cpp
1: #include "server.h"
2: #include "utils.h"
3: #include "protocol.h"
4: #include <cstdio>
5: #include <cstring>
6: #include <netinet/in.h>
7: #include <netinet/tcp.h>
8: #include <stdexcept>
9: #include <sys/epoll.h>
10: #include <sys/socket.h>
11: #include <unistd.h>
12: 
13: Server::Server(int port) : port(port), server_fd(-1), epfd(-1) {
14:   server_fd = socket(AF_INET, SOCK_STREAM, 0);
15:   if (server_fd < 0) {
16:     throw std::runtime_error("Failed to create socket");
17:   }
18: 
19:   int opt = 1;
20:   setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
21: 
22:   sockaddr_in addr{};
23:   addr.sin_family = AF_INET;
24:   addr.sin_addr.s_addr = INADDR_ANY;
25:   addr.sin_port = htons(port);
26: 
27:   if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
28:     throw std::runtime_error("Failed to bind");
29:   }
30: 
31:   if (listen(server_fd, 128) < 0) {
32:     throw std::runtime_error("Failed to listen");
33:   }
34: 
35:   set_nonblocking(server_fd);
36: 
37:   epfd = epoll_create1(0);
38:   if (epfd < 0) {
39:     throw std::runtime_error("Failed to create epoll");
40:   }
41: 
42:   epoll_event ev{};
43:   ev.events = EPOLLIN;
44:   ev.data.fd = server_fd;
45:   epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
46: }
47: 
48: Server::~Server() {
49:   if (server_fd >= 0)
50:     close(server_fd);
51:   if (epfd >= 0)
52:     close(epfd);
53: }
54: 
55: void Server::run() {
56:   epoll_event events[MAX_EVENTS];
57:   printf("listening on port %d\n", port);
58: 
59:   while (true) {
60:     int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
61:     for (int i = 0; i < n; i++) {
62:       int fd = events[i].data.fd;
63:       if (fd == server_fd) {
64:         accept_clients();
65:       } else {
66:         if (events[i].events & EPOLLIN) {
67:           handle_read(fd);
68:         }
69:         if (events[i].events & EPOLLOUT) {
70:           handle_write(fd);
71:         }
72:       }
73:     }
74:   }
75: }
76: 
77: void Server::accept_clients() {
78:   while (true) {
79:     int client_fd = accept(server_fd, nullptr, nullptr);
80:     if (client_fd < 0)
81:       break;
82:     clients[client_fd] = ClientState{client_fd, "", ""};
83:     set_nonblocking(client_fd);
84:     int nodelay = 1;
85:     setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
86: 
87:     epoll_event cev{};
88:     cev.events = EPOLLIN;
89:     cev.data.fd = client_fd;
90:     epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
91: #ifdef VERBOSE
92:     printf("client %d connected\n", client_fd);
93: #endif
94:   }
95: }
96: 
97: 
98: void Server::handle_read(int fd) {
99:   char buf[4096];
100:   ssize_t count = read(fd, buf, sizeof(buf));
101: 
102:   if (count <= 0) {
103:     if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
104:       return;
105:     }
106:     close_client(fd);
107:     return;
108:   }
109: 
110:   auto &state = clients[fd];
111:   state.read_buf.append(buf, count);
112: 
113:   constexpr size_t MAX_QUERY_LEN = 300;
114: 
115:   size_t pos;
116:   while ((pos = state.read_buf.find('\n')) != std::string::npos) {
117:     if (pos > MAX_QUERY_LEN) {
118:       printf("Client %d query exceeded MAX_QUERY_LEN (%zu bytes). Dropping connection.\n", fd, MAX_QUERY_LEN);
119:       close_client(fd);
120:       return;
121:     }
122: 
123:     std::string message = state.read_buf.substr(0, pos);
124:     state.read_buf.erase(0, pos + 1);
125: 
126:     handle_message(fd, message);
127:   }
128: 
129:   if (state.read_buf.size() > MAX_QUERY_LEN) {
130:     printf("Client %d buffer exceeded MAX_QUERY_LEN (%zu bytes). Dropping connection.\n", fd, MAX_QUERY_LEN);
131:     close_client(fd);
132:     return;
133:   }
134: 
135:   if (!state.write_buf.empty()) {
136:     ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
137:     if (written > 0) {
138:       state.write_buf.erase(0, written);
139:     } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
140:       close_client(fd);
141:       return;
142:     }
143: 
144:     if (!state.write_buf.empty()) {
145:       epoll_event cev{};
146:       cev.events = EPOLLIN | EPOLLOUT;
147:       cev.data.fd = fd;
148:       epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
149:     }
150:   }
151: }
152: 
153: void Server::handle_message(int fd, const std::string &message) {
154:   std::string_view msg_view = message;
155: 
156:   while (!msg_view.empty() && (msg_view.back() == '\r' || msg_view.back() == ' ')) {
157:     msg_view.remove_suffix(1);
158:   }
159:   while (!msg_view.empty() && (msg_view.front() == ' ')) {
160:     msg_view.remove_prefix(1);
161:   }
162: 
163:   if (msg_view.empty()) {
164:     return;
165:   }
166: 
167:   std::string response = Protocol::execute_command(msg_view);
168:   if (!response.empty()) {
169:     auto &state = clients[fd];
170:     state.write_buf += response;
171:   }
172: }
173: 
174: void Server::handle_write(int fd) {
175:   if (clients.find(fd) == clients.end())
176:     return;
177:   auto &state = clients[fd];
178: 
179:   ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
180:   if (written > 0) {
181:     state.write_buf.erase(0, written);
182:   } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
183:     close_client(fd);
184:     return;
185:   }
186: 
187:   if (state.write_buf.empty()) {
188:     epoll_event cev{};
189:     cev.events = EPOLLIN;
190:     cev.data.fd = fd;
191:     epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
192:   }
193: }
194: 
195: void Server::close_client(int fd) {
196:   epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
197:   close(fd);
198:   clients.erase(fd);
199: #ifdef VERBOSE
200:   printf("client %d disconnected\n", fd);
201: #endif
202: }
```

---

## Line-by-Line Breakdown & Explanation

### Header Inclusions (Lines 1–11)
- **Line 1: `#include "server.h"`**  
  Class interface containing declarations of `Server`, methods, and member attributes.
- **Line 2: `#include "utils.h"`**  
  Declares the `set_nonblocking(int fd)` socket utility.
- **Line 3: `#include "protocol.h"`**  
  Declares `Protocol::execute_command()`.
- **Line 4: `#include <cstdio>`**  
  Provides `printf` for console status messages.
- **Line 5: `#include <cstring>`**  
  Provides memory and string functions (e.g. `memset`).
- **Line 6: `#include <netinet/in.h>`**  
  Provides Internet address structures (`sockaddr_in`, `INADDR_ANY`) and byte-order conversions (`htons`).
- **Line 7: `#include <netinet/tcp.h>`**  
  Provides TCP socket options, specifically `TCP_NODELAY`.
- **Line 8: `#include <stdexcept>`**  
  Provides standard exception classes such as `std::runtime_error`.
- **Line 9: `#include <sys/epoll.h>`**  
  Provides Linux event polling system calls and structures (`epoll_create1`, `epoll_ctl`, `epoll_wait`, `struct epoll_event`, `EPOLLIN`, `EPOLLOUT`).
- **Line 10: `#include <sys/socket.h>`**  
  Provides standard POSIX networking calls (`socket`, `bind`, `listen`, `accept`, `setsockopt`, `SOL_SOCKET`, `SO_REUSEADDR`).
- **Line 11: `#include <unistd.h>`**  
  Provides standard POSIX system calls (`read`, `write`, `close`).

---

### Constructor: `Server::Server(int port)` (Lines 13–46)
- **Line 13: `Server::Server(int port) : port(port), server_fd(-1), epfd(-1) {`**  
  Initializes member variables: stores target port and defaults file descriptors to `-1` (invalid state).
- **Line 14: `  server_fd = socket(AF_INET, SOCK_STREAM, 0);`**  
  Creates an IPv4 (`AF_INET`) TCP byte-stream socket (`SOCK_STREAM`).
- **Line 15–17:**  
  Checks if `server_fd < 0`; if socket creation fails, throws `std::runtime_error`.
- **Line 19–20:**  
  ```cpp
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  ```
  Enables `SO_REUSEADDR` to allow immediate rebinding to port 6380 even if previous sockets remain in the kernel `TIME_WAIT` state.
- **Line 22–25:**  
  ```cpp
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  ```
  Zero-initializes `sockaddr_in` struct, specifies IPv4, binds to all local network interfaces (`INADDR_ANY`), and converts the port number to network byte order (big-endian) using `htons()`.
- **Line 27–29:**  
  Binds `server_fd` to the address. If `bind()` fails (e.g. port already occupied), throws `std::runtime_error`.
- **Line 31–33:**  
  Marks the socket as passive with a connection backlog queue of 128. If `listen()` fails, throws `std::runtime_error`.
- **Line 35: `  set_nonblocking(server_fd);`**  
  Sets the listening socket to `O_NONBLOCK` so `accept()` loops never hang.
- **Line 37–40:**  
  Calls `epoll_create1(0)` to initialize a new epoll kernel instance. Throws `std::runtime_error` if `epfd < 0`.
- **Line 42–45:**  
  ```cpp
  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
  ```
  Zero-initializes `epoll_event`, registers `server_fd` for read events (`EPOLLIN` indicates incoming client connections are waiting in the backlog), and adds it to the epoll watch list using `EPOLL_CTL_ADD`.
- **Line 46: `}`**  
  Ends constructor.

---

### Destructor: `Server::~Server()` (Lines 48–53)
- **Line 48: `Server::~Server() {`**  
  Class destructor ensuring proper RAII resource teardown.
- **Line 49–50: `  if (server_fd >= 0) close(server_fd);`**  
  Closes master listening socket if it was opened.
- **Line 51–52: `  if (epfd >= 0) close(epfd);`**  
  Closes epoll kernel instance descriptor.
- **Line 53: `}`**  
  Ends destructor.

---

### Event Loop: `Server::run()` (Lines 55–75)
- **Line 55: `void Server::run() {`**  
  The reactor event loop.
- **Line 56: `  epoll_event events[MAX_EVENTS];`**  
  Stack-allocated array of 64 `epoll_event` structs receiving triggered events from the kernel.
- **Line 57: `  printf("listening on port %d\n", port);`**  
  Prints startup confirmation message to standard output.
- **Line 59: `  while (true) {`**  
  Infinite event polling loop.
- **Line 60: `    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);`**  
  Blocks until one or more watched descriptors become ready for I/O (`-1` timeout means wait indefinitely without spinning the CPU). Returns the count of ready events `n`.
- **Line 61: `    for (int i = 0; i < n; i++) {`**  
  Iterates over each ready event in the batch.
- **Line 62: `      int fd = events[i].data.fd;`**  
  Retrieves the file descriptor associated with the event.
- **Line 63–64: `      if (fd == server_fd) { accept_clients(); }`**  
  If the ready descriptor is the listening socket, calls `accept_clients()` to accept incoming connections.
- **Line 65: `      } else {`**  
  Otherwise, the event corresponds to an active client socket.
- **Line 66–68: `        if (events[i].events & EPOLLIN) { handle_read(fd); }`**  
  If `EPOLLIN` is set, data is available in the socket's receive buffer; invokes `handle_read(fd)`.
- **Line 69–71: `        if (events[i].events & EPOLLOUT) { handle_write(fd); }`**  
  If `EPOLLOUT` is set, the socket's transmit buffer is ready to accept outgoing bytes; invokes `handle_write(fd)`.
- **Line 72–74:**  
  Closes `for` and `while` loops.
- **Line 75: `}`**  
  Ends `run()`.

---

### Connection Acceptor: `Server::accept_clients()` (Lines 77–95)
- **Line 77: `void Server::accept_clients() {`**  
  Handles new client connections.
- **Line 78: `  while (true) {`**  
  Loops until all pending connections in the kernel listen queue are accepted.
- **Line 79: `    int client_fd = accept(server_fd, nullptr, nullptr);`**  
  Extracts the first connection request on the queue.
- **Line 80–81: `    if (client_fd < 0) break;`**  
  Because `server_fd` is non-blocking, when the queue is exhausted, `accept()` returns `-1` with `errno = EAGAIN` or `EWOULDBLOCK`, cleanly exiting the loop.
- **Line 82: `    clients[client_fd] = ClientState{client_fd, "", ""};`**  
  Creates a new `ClientState` entry in the `clients` hash map with empty read/write buffers.
- **Line 83: `    set_nonblocking(client_fd);`**  
  Sets the client socket to `O_NONBLOCK`.
- **Line 84–85:**  
  ```cpp
  int nodelay = 1;
  setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  ```
  Disables Nagle's algorithm via `TCP_NODELAY`. This forces small command packets to transmit immediately without waiting for cumulative acknowledgments, eliminating 40ms delayed-ACK latencies.
- **Line 87–90:**  
  ```cpp
  epoll_event cev{};
  cev.events = EPOLLIN;
  cev.data.fd = client_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
  ```
  Registers the client descriptor with the epoll instance for read events (`EPOLLIN`).
- **Line 91–93:**  
  Conditionally prints connection confirmation if compiled with `-DVERBOSE`.
- **Line 94: `  }`**  
  Ends accept loop.
- **Line 95: `}`**  
  Ends `accept_clients()`.

---

### Ingress & Protocol Framing: `Server::handle_read(int fd)` (Lines 98–151)
- **Line 98: `void Server::handle_read(int fd) {`**  
  Called when data is ready to be read from a client socket.
- **Line 99: `  char buf[4096];`**  
  Stack buffer of 4 KB for reading incoming chunks.
- **Line 100: `  ssize_t count = read(fd, buf, sizeof(buf));`**  
  Executes non-blocking `read()` from the socket descriptor.
- **Line 102–108:**  
  Handles disconnects and errors:
  - If `count == 0`, client closed connection gracefully (`EOF`); calls `close_client(fd)`.
  - If `count < 0` with `errno == EAGAIN || errno == EWOULDBLOCK`, no more data is currently available; returns.
  - If `count < 0` with any other error (e.g. `ECONNRESET`), calls `close_client(fd)` to release resources.
- **Line 110–111:**  
  ```cpp
  auto &state = clients[fd];
  state.read_buf.append(buf, count);
  ```
  Appends newly read bytes to the client's persistent `read_buf`.
- **Line 113: `  constexpr size_t MAX_QUERY_LEN = 300;`**  
  Slowloris DoS Defense threshold: Maximum permitted command line size (300 bytes).
- **Line 115–127:**  
  Delimiter-based application framing loop:
  - Line 116: Scans for newline delimiter `\n` in `read_buf`.
  - Line 117–121: If line length before `\n` exceeds `MAX_QUERY_LEN`, drops connection immediately to protect server memory.
  - Line 123: Slices out complete command string `message = state.read_buf.substr(0, pos)`.
  - Line 124: Erases the parsed message and delimiter from `read_buf`.
  - Line 126: Dispatches message to `handle_message(fd, message)`.
- **Line 129–133:**  
  Checks if remaining incomplete buffer exceeds `MAX_QUERY_LEN` without having sent a `\n` (slow drip attack). If exceeded, logs and drops connection via `close_client(fd)`.
- **Line 135–150:**  
  Immediate flush attempt of queued response bytes:
  - Line 136: Calls non-blocking `write(fd, write_buf.data(), write_buf.size())`.
  - Line 137–138: Erases successfully written bytes from `write_buf`.
  - Line 139–142: If write fails fatally (not `EAGAIN`/`EWOULDBLOCK`), closes client.
  - Line 144–149: If bytes still remain unsent in `write_buf`, modifies the descriptor's epoll registration to `EPOLLIN | EPOLLOUT` using `EPOLL_CTL_MOD` so the reactor is notified as soon as the socket is writable again.
- **Line 151: `}`**  
  Ends `handle_read()`.

---

### Message Execution: `Server::handle_message(int fd, const std::string &message)` (Lines 153–172)
- **Line 153: `void Server::handle_message(int fd, const std::string &message) {`**  
  Dispatches a single complete framed command string.
- **Line 154: `  std::string_view msg_view = message;`**  
  Converts message to zero-copy `std::string_view`.
- **Line 156–161:**  
  Trims trailing `\r` and whitespace from both ends of the view using `remove_suffix()` and `remove_prefix()`.
- **Line 163–165: `  if (msg_view.empty()) return;`**  
  Skips empty commands.
- **Line 167: `  std::string response = Protocol::execute_command(msg_view);`**  
  Executes command via zero-copy protocol parser.
- **Line 168–171:**  
  If response is not empty, appends response to `clients[fd].write_buf`.
- **Line 172: `}`**  
  Ends `handle_message()`.

---

### Egress Backpressure Handler: `Server::handle_write(int fd)` (Lines 174–193)
- **Line 174: `void Server::handle_write(int fd) {`**  
  Invoked by reactor when a socket with pending writes becomes writable (`EPOLLOUT`).
- **Line 175–176: `  if (clients.find(fd) == clients.end()) return;`**  
  Safety guard ensuring descriptor still exists in `clients`.
- **Line 177: `  auto &state = clients[fd];`**  
  Gets reference to client state.
- **Line 179: `  ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());`**  
  Performs non-blocking write to socket.
- **Line 180–182:**  
  Erases sent bytes from `write_buf`.
- **Line 182–185:**  
  Closes client if write returns fatal socket error.
- **Line 187–192:**  
  ```cpp
  if (state.write_buf.empty()) {
    epoll_event cev{};
    cev.events = EPOLLIN;
    cev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
  }
  ```
  Once `write_buf` is completely drained, modifies epoll event registration back to `EPOLLIN` only. This is critical to avoid CPU 100% busy-spinning, since a non-full socket transmit buffer will constantly trigger `EPOLLOUT` if left registered.
- **Line 193: `}`**  
  Ends `handle_write()`.

---

### Teardown: `Server::close_client(int fd)` (Lines 195–202)
- **Line 195: `void Server::close_client(int fd) {`**  
  Gracefully releases all client resources.
- **Line 196: `  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);`**  
  Deregisters socket descriptor from the epoll watch set.
- **Line 197: `  close(fd);`**  
  Closes the underlying OS socket file descriptor.
- **Line 198: `  clients.erase(fd);`**  
  Deletes the client's `ClientState` entry and deallocates its buffers from the hash map.
- **Line 199–201:**  
  Logs disconnection if `-DVERBOSE` is enabled.
- **Line 202: `}`**  
  Ends `close_client()`.
