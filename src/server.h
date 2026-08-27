#ifndef SERVER_H
#define SERVER_H

#include <unordered_map>
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
    std::unordered_map<int, ClientState> clients;
    
    static constexpr int MAX_EVENTS = 64;
};

#endif // SERVER_H
