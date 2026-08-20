#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <string>

constexpr int MAX_EVENTS = 64;
constexpr int PORT = 6380;

struct ClientState {
    std::string read_buf;
    std::string write_buf;
};
std::unordered_map<int, ClientState> clients;

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 128);
    set_nonblocking(server_fd);

    int epfd = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    epoll_event events[MAX_EVENTS];
    printf("listening on port %d\n", PORT);

    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                while (true) {
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd < 0) break; 
                    clients[client_fd] = ClientState{};
                    set_nonblocking(client_fd);
                    epoll_event cev{};
                    cev.events = EPOLLIN;
                    cev.data.fd = client_fd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
                    printf("client %d connected\n", client_fd);
                }
                } else {
                    if (events[i].events & EPOLLIN) {
                        char buf[4096];
                        ssize_t count = read(fd, buf, sizeof(buf));

                        if (count <= 0) {
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                            clients.erase(fd);
                            continue;
                        } else {
                            auto& state = clients[fd];
                            state.read_buf.append(buf, count);
                            state.write_buf += state.read_buf;
                            state.read_buf.clear();

                            ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
                            if (written > 0) {
                                state.write_buf.erase(0, written);
                            }

                            if (!state.write_buf.empty()) {
                                epoll_event cev{};
                                cev.events = EPOLLIN | EPOLLOUT;
                                cev.data.fd = fd;
                                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
                            }
                        }
                    }

                    if (events[i].events & EPOLLOUT) {
                        auto& state = clients[fd];
                        ssize_t written = write(fd, state.write_buf.data(), state.write_buf.size());
                        if (written > 0) {
                            state.write_buf.erase(0, written);
                        }
                        if (state.write_buf.empty()) {
                            epoll_event cev{};
                            cev.events = EPOLLIN;
                            cev.data.fd = fd;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &cev);
                        }
                    }
                }
        }
    }
}
