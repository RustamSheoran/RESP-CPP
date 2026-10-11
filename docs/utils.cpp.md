# Documentation: `src/utils.cpp`

The [`src/utils.cpp`](file:///home/rustam/Projects/resp-cpp/src/utils.cpp) file implements socket configuration utilities.

---

### Line 1: `#include "utils.h"`
Includes the header file declaring `set_nonblocking(int fd)` so the compiler verifies type signatures and function prototypes.

### Line 2: `#include <fcntl.h>`
Imports the POSIX file control header defining the `fcntl()` system call and constants `F_GETFL`, `F_SETFL`, and `O_NONBLOCK`.

### Line 3: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 4: `void set_nonblocking(int fd) {`
Defines the `set_nonblocking` function, accepting an open file descriptor integer `fd` representing a socket.

### Line 5: `    int flags = fcntl(fd, F_GETFL, 0);`
Calls the `fcntl` system call with command `F_GETFL` to query and retrieve the current file status bitmask flags of the descriptor.

### Line 6: `    fcntl(fd, F_SETFL, flags | O_NONBLOCK);`
Bitwise-ORs the retrieved flags with the `O_NONBLOCK` bit and applies them to the kernel file descriptor table using `F_SETFL`. This guarantees future `read()`, `write()`, and `accept()` calls will return immediately with `EAGAIN`/`EWOULDBLOCK` instead of blocking the reactor thread.

### Line 7: `}`
Closes the function definition of `set_nonblocking`.
