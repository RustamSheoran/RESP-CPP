#include "server.h"
#include "utils.h"
#include "protocol.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port) : port(port), server_fd(-1), epfd(-1), idle_fd(-1) {
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    throw std::runtime_error("Failed to bind");
  }

  if (listen(server_fd, 128) < 0) {
    throw std::runtime_error("Failed to listen");
  }

  set_nonblocking(server_fd);

  epfd = epoll_create1(0);
  if (epfd < 0) {
    throw std::runtime_error("Failed to create epoll");
  }

  idle_fd = open("/dev/null", O_RDONLY);

  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
}

Server::~Server() {
  if (server_fd >= 0)
    close(server_fd);
  if (epfd >= 0)
    close(epfd);
  if (idle_fd >= 0)
    close(idle_fd);
}

void Server::run() {
  epoll_event events[MAX_EVENTS];
  printf("listening on port %d\n", port);

  while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, 1000);
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;
      if (fd == server_fd) {
        accept_clients();
      } else {
        if (events[i].events & EPOLLIN) {
          handle_read(fd);
        }
        if (events[i].events & EPOLLOUT) {
          handle_write(fd);
        }
      }
    }

    auto now = std::chrono::steady_clock::now();
    std::vector<int> to_close;
    for (const auto &[cfd, state] : clients) {
      if (std::chrono::duration_cast<std::chrono::seconds>(now - state.last_active).count() > IDLE_TIMEOUT_SECONDS) {
        to_close.push_back(cfd);
      }
    }
    for (int cfd : to_close) {
      close_client(cfd);
    }
  }
}

void Server::accept_clients() {
  while (true) {
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
      if (errno == EMFILE || errno == ENFILE) {
        if (idle_fd >= 0) {
          close(idle_fd);
          int temp_fd = accept(server_fd, nullptr, nullptr);
          if (temp_fd >= 0) close(temp_fd);
          idle_fd = open("/dev/null", O_RDONLY);
        }
      }
      break;
    }
    ClientState state{client_fd, "", "", false, std::chrono::steady_clock::now()};
    state.read_buf.reserve(16384);
    state.write_buf.reserve(16384);
    clients[client_fd] = std::move(state);
    set_nonblocking(client_fd);
    int nodelay = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    epoll_event cev{};
    cev.events = EPOLLIN;
    cev.data.fd = client_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
#ifdef VERBOSE
    printf("client %d connected\n", client_fd);
#endif
  }
}

void Server::handle_read(int fd) {
  char buf[16384];
  ssize_t count = read(fd, buf, sizeof(buf));

  if (count <= 0) {
    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return;
    }
    close_client(fd);
    return;
  }

  auto &state = clients[fd];
  state.last_active = std::chrono::steady_clock::now();
  state.read_buf.append(buf, count);

  size_t cursor = 0;
  while (cursor < state.read_buf.size()) {
    bool should_close = false;
    auto res = Protocol::parse_and_execute(state.read_buf, cursor, state.write_buf, should_close);
    if (res == Protocol::ParseResult::NEED_MORE_DATA) {
      break;
    }
    if (should_close) {
      state.should_close = true;
    }
    if (state.write_buf.size() > MAX_WRITE_BUF_LEN) {
      close_client(fd);
      return;
    }
  }

  if (cursor > 0) {
    state.read_buf.erase(0, cursor);
  }

  if (!state.write_buf.empty()) {
    ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
    if (written > 0) {
      state.write_buf.erase(0, written);
    } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      close_client(fd);
      return;
    }
  }

  if (state.should_close && state.write_buf.empty()) {
    close_client(fd);
    return;
  }

  if (!state.write_buf.empty()) {
    epoll_event cev{};
    cev.events = EPOLLIN | EPOLLOUT;
    cev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
  }
}

void Server::handle_write(int fd) {
  if (clients.find(fd) == clients.end())
    return;
  auto &state = clients[fd];

  ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
  if (written > 0) {
    state.write_buf.erase(0, written);
  } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    close_client(fd);
    return;
  }

  if (state.write_buf.empty()) {
    if (state.should_close) {
      close_client(fd);
      return;
    }
    epoll_event cev{};
    cev.events = EPOLLIN;
    cev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
  }
}

void Server::close_client(int fd) {
  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
  close(fd);
  clients.erase(fd);
#ifdef VERBOSE
  printf("client %d disconnected\n", fd);
#endif
}
