#ifndef CLIENT_H
#define CLIENT_H

#include <chrono>
#include <string>

struct ClientState {
    int fd;
    std::string read_buf;
    std::string write_buf;
    bool should_close;
    std::chrono::steady_clock::time_point last_active;
};

#endif
