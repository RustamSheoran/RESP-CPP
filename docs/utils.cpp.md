# Documentation: `src/utils.cpp`

The [`src/utils.cpp`](file:///home/rustam/Projects/resp-cpp/src/utils.cpp) source file implements low-level POSIX file-descriptor manipulation helpers, specifically configuring non-blocking I/O via the Linux `fcntl` system call.

---

## Complete Source Code

```cpp
1: #include "utils.h"
2: #include <fcntl.h>
3: 
4: void set_nonblocking(int fd) {
5:     int flags = fcntl(fd, F_GETFL, 0);
6:     fcntl(fd, F_SETFL, flags | O_NONBLOCK);
7: }
```

---

## Line-by-Line Breakdown & Explanation

### Line 1: `#include "utils.h"`
- **Purpose:** Includes the associated header file.
- **Detailed Explanation:** Guarantees that the function definition in this translation unit matches the forward declaration in [`src/utils.h`](file:///home/rustam/Projects/resp-cpp/src/utils.h). The compiler validates parameter types (`int fd`) and the return type (`void`).

### Line 2: `#include <fcntl.h>`
- **Purpose:** POSIX file control header.
- **Detailed Explanation:** Imports standard POSIX system definitions for the `fcntl()` system call and related constants:
  - `F_GETFL`: Command to retrieve file access modes and file status flags.
  - `F_SETFL`: Command to set the file status flags to the value specified by the third argument.
  - `O_NONBLOCK`: The non-blocking I/O mode bitmask flag.

### Line 3: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 4: `void set_nonblocking(int fd) {`
- **Purpose:** Function definition header.
- **Detailed Explanation:** Defines `set_nonblocking`, accepting an open file descriptor integer `fd`. This function is invoked on the master listening socket (`server_fd`) and on every client socket accepted via `accept()`.

### Line 5: `    int flags = fcntl(fd, F_GETFL, 0);`
- **Purpose:** Query existing file status flags.
- **Detailed Explanation:** Calls the `fcntl` (file control) system call with the `F_GETFL` command. The third argument `0` is ignored for `F_GETFL`.
  - It retrieves the current bitmask of flags already set on this file descriptor (such as read/write permissions, append mode, etc.).
  - Preserving existing flags before modifying them is a critical POSIX best practice to avoid accidentally overwriting or clearing other socket attributes.

### Line 6: `    fcntl(fd, F_SETFL, flags | O_NONBLOCK);`
- **Purpose:** Enable the `O_NONBLOCK` status flag.
- **Detailed Explanation:** Uses bitwise-OR (`flags | O_NONBLOCK`) to combine the descriptor's existing flags with the `O_NONBLOCK` bit, then writes the updated mask back into the kernel table for `fd` using `F_SETFL`.
  - **Effect on Sockets:**
    - Any future `read()` calls on this socket will return immediately with `-1` and set `errno = EAGAIN` or `EWOULDBLOCK` if the socket receive buffer is empty, rather than blocking the calling thread.
    - Any future `write()` calls will write as many bytes as will fit in the socket transmit buffer and immediately return without waiting for network ACK or buffer clearance.
    - Any future `accept()` calls will return `-1` with `EAGAIN` if the kernel listen queue is empty, enabling non-blocking connection loops.

### Line 7: `}`
- **Purpose:** Closing delimiter of `set_nonblocking`.
- **Detailed Explanation:** Terminates the function body and returns control to the caller.
