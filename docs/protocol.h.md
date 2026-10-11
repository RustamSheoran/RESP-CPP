# Documentation: `src/protocol.h`

The [`src/protocol.h`](file:///home/rustam/Projects/resp-cpp/src/protocol.h) header declares the protocol parser interface, security bounds constants, and status result types.

---

### Line 1: `#ifndef PROTOCOL_H`
Header include guard preventing multiple compilation passes over this file within the same translation unit.

### Line 2: `#define PROTOCOL_H`
Defines `PROTOCOL_H` macro to enforce header guard semantics.

### Line 3: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 4: `#include <string>`
Imports `std::string` used for input buffers and serialized RESP response payloads.

### Line 5: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 6: `namespace Protocol {`
Opens the `Protocol` namespace boundary to encapsulate parser functions and database logic.

### Line 7: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 8: `constexpr long long MAX_MULTIBULK_ARGS = 1024 * 1024;`
Defines the maximum allowed number of multibulk arguments ($1,048,576$) per command frame, preventing memory exhaustion and denial-of-service crashes from maliciously large array counts.

### Line 9: `constexpr long long MAX_BULK_LEN = 512 * 1024 * 1024;`
Defines the maximum allowed length ($512\text{ MB}$) for individual bulk string arguments matching standard Redis protocol limits, rejecting oversized payload declarations early.

### Line 10: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 11: `enum class ParseResult {`
Declares the scoped enumeration `ParseResult` for type-safe status reporting from the streaming parser.

### Line 12: `    SUCCESS,`
Indicates a full command frame was successfully extracted, parsed, and executed.

### Line 13: `    NEED_MORE_DATA`
Indicates that the buffer contains an incomplete command or truncated multibulk payload, signaling the reactor loop to wait for further incoming network packets.

### Line 14: `};`
Terminates the `ParseResult` enumeration declaration.

### Line 15: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 16: `ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close);`
Function declaration for `parse_and_execute`. Slices commands from `buf` starting at `cursor` (supporting both RESP multibulk arrays `*N\r\n...` and inline commands `\n`), executes commands with zero-copy argument views, appends serialized RESP replies directly into `write_buf`, advances `cursor` past consumed bytes, and sets `should_close = true` if `QUIT` or a fatal protocol error occurred.

### Line 17: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 18: `}`
Closes `namespace Protocol`.

### Line 19: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 20: `#endif`
Terminates the `#ifndef PROTOCOL_H` header guard block.
