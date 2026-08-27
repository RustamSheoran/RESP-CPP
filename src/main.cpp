#include "server.h"
#include <iostream>

constexpr int PORT = 6380;

int main() {
    try {
        Server server(PORT);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
