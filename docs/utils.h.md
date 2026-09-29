# Documentation: `src/utils.h`

The [`src/utils.h`](file:///home/rustam/Projects/resp-cpp/src/utils.h) header file provides the declaration for socket utility functions used throughout the RESP server, specifically placing file descriptors into non-blocking mode.

---

## Complete Source Code

```cpp
1: #ifndef UTILS_H
2: #define UTILS_H
3: 
4: void set_nonblocking(int fd);
5: 
6: #endif
```

---

## Line-by-Line Breakdown & Explanation

### Line 1: `#ifndef UTILS_H`
- **Purpose:** Preprocessor conditional directive ("if not defined").
- **Detailed Explanation:** This is the beginning of a standard C/C++ header guard. It checks if the preprocessor macro `UTILS_H` has not yet been defined in the current translation unit. If multiple source files include `utils.h`, this prevents the header's contents from being declared more than once, which would trigger compilation errors such as redefinition of functions or types.

### Line 2: `#define UTILS_H`
- **Purpose:** Preprocessor macro definition.
- **Detailed Explanation:** When `UTILS_H` is not yet defined, this line defines it. Any subsequent `#include "utils.h"` directives encountered in the same translation unit will see that `UTILS_H` is already defined and skip lines 3–5 entirely until `#endif`.

### Line 3: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 4: `void set_nonblocking(int fd);`
- **Purpose:** Function forward declaration.
- **Detailed Explanation:**
  - `void`: The function does not return any value.
  - `set_nonblocking`: Name of the helper function implemented in [`src/utils.cpp`](file:///home/rustam/Projects/resp-cpp/src/utils.cpp).
  - `int fd`: Takes a single argument representing a POSIX file descriptor (in this project, either the listening socket `server_fd` or an accepted client connection socket `client_fd`).
  - This function modifies the file descriptor's flags using the `fcntl` system call to configure it for asynchronous, non-blocking I/O (`O_NONBLOCK`). Non-blocking mode is essential for single-threaded `epoll` reactors because it ensures system calls like `read()`, `write()`, and `accept()` never freeze or suspend the thread when no data is ready.

### Line 5: *(Empty Line)*
- **Purpose:** Visual separation for readability.

### Line 6: `#endif`
- **Purpose:** Preprocessor conditional terminator.
- **Detailed Explanation:** Closes the `#ifndef UTILS_H` conditional block started on Line 1. Any code after this line would be included regardless of whether `UTILS_H` was defined, though here it signifies the end of the header file.
