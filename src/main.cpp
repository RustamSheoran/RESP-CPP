#include "server.h"
#include <csignal>
#include <iostream>

constexpr int PORT = 6380;

int main() {
  // Phase 9: Ignore SIGPIPE so dead socket writes return -1 (EPIPE) instead of crashing the process
  signal(SIGPIPE, SIG_IGN);

  try {
    Server server(PORT);
    server.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
