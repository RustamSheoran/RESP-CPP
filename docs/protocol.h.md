# Documentation: `src/protocol.h`

The [`src/protocol.h`](file:///home/rustam/Projects/resp-cpp/src/protocol.h) header declares the protocol parser interface and status result types.

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

### Line 8: `enum class ParseResult {`
Declares the scoped enumeration `ParseResult` for type-safe status reporting from the streaming parser.

### Line 9: `    SUCCESS,`
Indicates a full command frame was successfully extracted, parsed, and executed.

### Line 10: `    NEED_MORE_DATA`
Indicates that the buffer contains an incomplete command or truncated multibulk payload, signaling the reactor loop to wait for further incoming network packets.

### Line 11: `};`
Terminates the `ParseResult` enumeration declaration.

### Line 12: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 13: `ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close);`
Function declaration for `parse_and_execute`. Slices commands from `buf` starting at `cursor` (supporting both RESP multibulk arrays `*N\r\n...` and inline commands `\n`), executes commands with zero-copy argument views, appends serialized RESP replies directly into `write_buf`, advances `cursor` past consumed bytes, and sets `should_close = true` if `QUIT` was executed.

### Line 14: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 15: `}`
Closes `namespace Protocol`.

### Line 16: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 17: `#endif`
Terminates the `#ifndef PROTOCOL_H` header guard block.
