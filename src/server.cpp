#include "server.h"
#include "utils.h"
#include "protocol.h"
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port) : port(port), server_fd(-1), epfd(-1) {
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
}

void Server::run() {
  epoll_event events[MAX_EVENTS];
  printf("listening on port %d\n", port);

  while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
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
  }
}

void Server::accept_clients() {
  while (true) {
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0)
      break;
    clients[client_fd] = ClientState{client_fd, "", ""};
    set_nonblocking(client_fd);
    epoll_event cev{};
    cev.events = EPOLLIN;
    cev.data.fd = client_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
    printf("client %d connected\n", client_fd);
  }
}

void Server::handle_read(int fd) {
  char buf[4096];
  ssize_t count = read(fd, buf, sizeof(buf));

  if (count <= 0) {
    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return;
    }
    close_client(fd);
    return;
  }

  auto &state = clients[fd];
  state.read_buf.append(buf, count);

  // Phase 6: Delimiter-based application message framing ('\n')
  size_t pos;
  while ((pos = state.read_buf.find('\n')) != std::string::npos) {
    std::string message = state.read_buf.substr(0, pos);
    state.read_buf.erase(0, pos + 1);

    handle_message(fd, message);
  }

  // Attempt to write out queued responses
  if (!state.write_buf.empty()) {
    ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
    if (written > 0) {
      state.write_buf.erase(0, written);
    } else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
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
}

void Server::handle_message(int fd, const std::string &message) {
  std::string_view msg_view = message;

  // Trim trailing carriage returns or whitespace
  while (!msg_view.empty() && (msg_view.back() == '\r' || msg_view.back() == ' ')) {
    msg_view.remove_suffix(1);
  }
  while (!msg_view.empty() && (msg_view.front() == ' ')) {
    msg_view.remove_prefix(1);
  }

  if (msg_view.empty()) {
    return;
  }

  // Phase 10: Zero-copy execution of Redis commands (PING, SET, GET, DEL)
  std::string response = Protocol::execute_command(msg_view);
  if (!response.empty()) {
    auto &state = clients[fd];
    state.write_buf += response;
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
  printf("client %d disconnected\n", fd);
}

