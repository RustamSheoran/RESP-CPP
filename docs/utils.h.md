# Documentation: `src/utils.h`

The [`src/utils.h`](file:///home/rustam/Projects/resp-cpp/src/utils.h) header declares utility helper functions for socket configuration.

---

### Line 1: `#ifndef UTILS_H`
Standard preprocessor conditional directive ("if not defined") initiating the header include guard to prevent multiple definitions of declarations during compilation.

### Line 2: `#define UTILS_H`
Defines the `UTILS_H` preprocessor macro so subsequent inclusions of this header file within the same translation unit skip its body.

### Line 3: *(Empty Line)*
Whitespace separator for visual clarity and file structure.

### Line 4: `void set_nonblocking(int fd);`
Function prototype declaring `set_nonblocking`, which takes an integer file descriptor `fd` (either the listening socket or an accepted client connection) and configures it for non-blocking I/O (`O_NONBLOCK`).

### Line 5: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 6: `#endif`
Terminates the `#ifndef UTILS_H` include guard conditional block.
