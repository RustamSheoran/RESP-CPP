#ifndef CLIENT_H
#define CLIENT_H

#include <string>

struct ClientState {
    int fd;
    std::string read_buf;
    std::string write_buf;
};

#endif // CLIENT_H
