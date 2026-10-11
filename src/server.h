#ifndef SERVER_H
#define SERVER_H

#include <unordered_map>
#include <chrono>
#include "client.h"

class Server {
public:
    Server(int port);
    ~Server();
    void run();

private:
    void accept_clients();
    void handle_read(int client_fd);
    void handle_write(int client_fd);
    void close_client(int client_fd);

    int port;
    int server_fd;
    int epfd;
    int idle_fd;
    std::unordered_map<int, ClientState> clients;
    std::chrono::steady_clock::time_point last_sweep;
    
    static constexpr int MAX_EVENTS = 64;
    static constexpr size_t MAX_WRITE_BUF_LEN = 10 * 1024 * 1024;
    static constexpr size_t MAX_READ_BUF_LEN = 10 * 1024 * 1024;
    static constexpr int IDLE_TIMEOUT_SECONDS = 60;
};

#endif
