# Documentation: `src/protocol.cpp`

The [`src/protocol.cpp`](file:///home/rustam/Projects/resp-cpp/src/protocol.cpp) file implements the dual-mode Redis command parser, quote-aware tokenizer, in-memory key-value database, bounded multibulk parsing, zero-copy argument views, direct egress buffer appending, and command execution routines.

---

### Line 1: `#include "protocol.h"`
Imports the interface header to validate function prototypes and `ParseResult` definitions.

### Line 2: `#include <unordered_map>`
Imports `std::unordered_map` used to store the in-memory key-value database with $O(1)$ amortized lookup.

### Line 3: `#include <vector>`
Imports `std::vector` to store parsed token argument arrays.

### Line 4: `#include <string_view>`
Imports `std::string_view` for zero-copy substring scanning and argument referencing.

### Line 5: `#include <cctype>`
Imports `std::toupper` for case-insensitive string comparisons.

### Line 6: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 7: `namespace Protocol {`
Opens `namespace Protocol` scoping all protocol-related logic.

### Line 8: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 9: `static std::unordered_map<std::string, std::string> db;`
Defines the in-memory key-value database holding stored keys and values with static internal linkage.

### Line 10: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 11: `static bool equals_ci(std::string_view a, std::string_view b) {`
Declares static helper function comparing two string views case-insensitively.

### Line 12: `    if (a.size() != b.size()) return false;`
Rejects comparisons immediately if string lengths differ.

### Line 13: `    for (size_t i = 0; i < a.size(); ++i) {`
Iterates over each character index of both string views simultaneously.

### Line 14: `        if (std::toupper(static_cast<unsigned char>(a[i])) !=`
### Line 15: `            std::toupper(static_cast<unsigned char>(b[i]))) {`
Compares uppercase-converted characters using `std::toupper`.

### Line 16: `            return false;`
Returns `false` upon finding the first mismatched character.

### Line 17: `        }`
Closes character mismatch condition.

### Line 18: `    }`
Closes comparison loop.

### Line 19: `    return true;`
Returns `true` if all characters match case-insensitively.

### Line 20: `}`
Closes `equals_ci`.

### Line 21: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 22: `static std::vector<std::string> tokenize_inline(std::string_view raw_cmd) {`
Tokenizes inline newline-terminated commands, supporting quoted arguments with spaces and escape characters.

### Line 23: `    std::vector<std::string> tokens;`
Allocates vector of extracted token strings.

### Line 24: `    size_t i = 0;`
Initializes character scan index.

### Line 25: `    while (i < raw_cmd.size()) {`
Loops through the command string until the end is reached.

### Line 26: `        while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {`
### Line 27: `            i++;`
### Line 28: `        }`
Skips leading whitespace and carriage return characters before each token.

### Line 29: `        if (i >= raw_cmd.size()) break;`
Terminates scan if no further tokens remain.

### Line 30: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 31: `        if (raw_cmd[i] == '"' || raw_cmd[i] == '\'') {`
Checks if the current argument starts with a single or double quote.

### Line 32: `            char quote = raw_cmd[i];`
Records the matching delimiter quote character.

### Line 33: `            i++;`
Advances index past the opening quote.

### Line 34: `            size_t start = i;`
Records start position of the quoted content.

### Line 35: `            while (i < raw_cmd.size() && raw_cmd[i] != quote) {`
Scans for closing quote, handling escaped characters.

### Line 36: `                if (raw_cmd[i] == '\\' && i + 1 < raw_cmd.size()) {`
### Line 37: `                    i += 2;`
### Line 38: `                } else {`
### Line 39: `                    i++;`
### Line 40: `                }`
Advances past backslash-escaped characters.

### Line 41: `            }`
Closes scanning loop for quoted string.

### Line 42: `            std::string tok;`
Initializes unescaped token string.

### Line 43: `            for (size_t k = start; k < i; ++k) {`
### Line 44: `                if (raw_cmd[k] == '\\' && k + 1 < i) {`
### Line 45: `                    tok.push_back(raw_cmd[k + 1]);`
### Line 46: `                    k++;`
### Line 47: `                } else {`
### Line 48: `                    tok.push_back(raw_cmd[k]);`
### Line 49: `                }`
### Line 50: `            }`
Appends unescaped characters into token string.

### Line 51: `            tokens.push_back(std::move(tok));`
Moves extracted unquoted string into tokens list.

### Line 52: `            if (i < raw_cmd.size() && raw_cmd[i] == quote) {`
### Line 53: `                i++;`
### Line 54: `            }`
Advances index past closing quote character.

### Line 55: `        } else {`
Handles standard unquoted word tokens.

### Line 56: `            size_t start = i;`
Records start index of unquoted token.

### Line 57: `            while (i < raw_cmd.size() && raw_cmd[i] != ' ' && raw_cmd[i] != '\t' && raw_cmd[i] != '\r') {`
### Line 58: `                i++;`
### Line 59: `            }`
Advances pointer until the next whitespace character.

### Line 60: `            tokens.emplace_back(raw_cmd.substr(start, i - start));`
Appends unquoted token slice into `tokens`.

### Line 61: `        }`
Closes quoted vs unquoted branching.

### Line 62: `    }`
Closes main tokenization loop.

### Line 63: `    return tokens;`
Returns list of extracted argument tokens.

### Line 64: `}`
Closes `tokenize_inline`.

### Line 65: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 66: `static bool parse_resp_multibulk(const std::string &buf, size_t cursor, size_t &consumed, std::vector<std::string_view> &args, bool &protocol_error) {`
Zero-copy RESP Multibulk Array parser extracting arguments as `std::string_view` directly into `args` without heap allocations, with strict security bounds.

### Line 67: `    protocol_error = false;`
Initializes protocol error flag to `false`.

### Line 68: `    if (cursor >= buf.size() || buf[cursor] != '*') {`
### Line 69: `        return false;`
### Line 70: `    }`
Validates that input begins with `*` at the current `cursor` position.

### Line 71: `    size_t line_end = buf.find("\r\n", cursor);`
### Line 72: `    if (line_end == std::string::npos) {`
### Line 73: `        return false;`
### Line 74: `    }`
Finds line terminator for argument count header; returns false (need more data) if incomplete.

### Line 75: `    long long num_args = 0;`
### Line 76: `    try {`
### Line 77: `        num_args = std::stoll(buf.substr(cursor + 1, line_end - (cursor + 1)));`
### Line 78: `    } catch (...) {`
### Line 79: `        protocol_error = true;`
### Line 80: `        consumed = line_end + 2 - cursor;`
### Line 81: `        return false;`
### Line 82: `    }`
Parses array argument count via `std::stoll`, flagging protocol error if number parsing throws.

### Line 83: `    if (num_args < 0 || num_args > MAX_MULTIBULK_ARGS) {`
### Line 84: `        protocol_error = true;`
### Line 85: `        consumed = line_end + 2 - cursor;`
### Line 86: `        return false;`
### Line 87: `    }`
Rejects negative numbers and counts exceeding `MAX_MULTIBULK_ARGS` ($1024 \times 1024$) as protocol errors, immunizing the server from remote out-of-memory crashes on inputs like `*99999999999\r\n`.

### Line 88: `    if (num_args == 0) {`
### Line 89: `        consumed = line_end + 2 - cursor;`
### Line 90: `        return true;`
### Line 91: `    }`
Handles empty array commands gracefully.

### Line 92: `    size_t pos = line_end + 2;`
Positions pointer at start of first bulk string argument.

### Line 93: `    std::vector<std::string_view> parsed;`
### Line 94: `    parsed.reserve(num_args);`
Allocates and reserves vector of string views safely after bounds verification.

### Line 95: `    for (long long i = 0; i < num_args; ++i) {`
Iterates through all array arguments.

### Line 96: `        if (pos >= buf.size() || buf[pos] != '$') {`
### Line 97: `            return false;`
### Line 98: `        }`
Validates `$` bulk string prefix.

### Line 99: `        size_t bulk_line_end = buf.find("\r\n", pos);`
### Line 100: `        if (bulk_line_end == std::string::npos) {`
### Line 101: `            return false;`
### Line 102: `        }`
Locates length terminator `\r\n`.

### Line 103: `        long long bulk_len = 0;`
### Line 104: `        try {`
### Line 105: `            bulk_len = std::stoll(buf.substr(pos + 1, bulk_line_end - (pos + 1)));`
### Line 106: `        } catch (...) {`
### Line 107: `            protocol_error = true;`
### Line 108: `            consumed = bulk_line_end + 2 - cursor;`
### Line 109: `            return false;`
### Line 110: `        }`
Parses bulk string byte length via `std::stoll`.

### Line 111: `        if (bulk_len < 0 || bulk_len > MAX_BULK_LEN) {`
### Line 112: `            protocol_error = true;`
### Line 113: `            consumed = bulk_line_end + 2 - cursor;`
### Line 114: `            return false;`
### Line 115: `        }`
Rejects negative lengths or lengths exceeding `MAX_BULK_LEN` ($512\text{ MB}$) as fatal protocol errors.

### Line 116: `        pos = bulk_line_end + 2;`
Advances pointer to bulk payload start.

### Line 117: `        if (pos + static_cast<size_t>(bulk_len) + 2 > buf.size()) {`
### Line 118: `            return false;`
### Line 119: `        }`
Checks that the entire payload plus framing `\r\n` has arrived. Returns false (need more data) if incomplete.

### Line 120: `        if (buf[pos + bulk_len] != '\r' || buf[pos + bulk_len + 1] != '\n') {`
### Line 121: `            protocol_error = true;`
### Line 122: `            consumed = pos + bulk_len + 2 - cursor;`
### Line 123: `            return false;`
### Line 124: `        }`
Verifies framing delimiter bytes `\r\n`, flagging protocol error if missing.

### Line 125: `        parsed.emplace_back(buf.data() + pos, bulk_len);`
Emplaces zero-copy string view referencing buffer memory directly.

### Line 126: `        pos += bulk_len + 2;`
Advances position past the payload and framing `\r\n`.

### Line 127: `    }`
Closes argument loop.

### Line 128: `    consumed = pos - cursor;`
Calculates total bytes consumed for this complete command frame.

### Line 129: `    args = std::move(parsed);`
Moves parsed token views array into output parameter.

### Line 130: `    return true;`
Returns `true` signaling command frame extraction succeeded.

### Line 131: `}`
Closes `parse_resp_multibulk`.

### Line 132: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 133: `static void execute_tokens(const std::vector<std::string_view> &tokens, std::string &write_buf, bool &should_close) {`
Command execution dispatcher executing commands against database and directly serializing RESP responses into `write_buf`.

### Line 134: `    if (tokens.empty()) {`
### Line 135: `        return;`
### Line 136: `    }`
Early return if argument list is empty.

### Line 137: `    std::string_view cmd = tokens[0];`
Extracts command name token view.

### Line 138: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 139: `    if (equals_ci(cmd, "PING")) {`
Checks for `PING` command case-insensitively.

### Line 140: `        if (tokens.size() == 1) {`
### Line 141: `            write_buf += "+PONG\r\n";`
Returns simple string `+PONG\r\n` for zero-argument `PING`.

### Line 142: `        } else if (tokens.size() == 2) {`
### Line 143: `            std::string_view msg = tokens[1];`
### Line 144: `            write_buf += "$" + std::to_string(msg.size()) + "\r\n";`
### Line 145: `            write_buf.append(msg.data(), msg.size());`
### Line 146: `            write_buf += "\r\n";`
Returns echoed message as bulk string `$<len>\r\n<msg>\r\n` directly appended without heap copies.

### Line 147: `        } else {`
### Line 148: `            write_buf += "-ERR wrong number of arguments for 'ping' command\r\n";`
### Line 149: `        }`
Returns error if extra arguments are supplied.

### Line 150: `        return;`
Exits execution after handling `PING`.

### Line 151: `    }`
Closes `PING` handler.

### Line 152: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 153: `    if (equals_ci(cmd, "SET")) {`
Checks for `SET` command case-insensitively.

### Line 154: `        if (tokens.size() != 3) {`
### Line 155: `            write_buf += "-ERR wrong number of arguments for 'set' command\r\n";`
### Line 156: `            return;`
### Line 157: `        }`
Strictly enforces exactly 2 arguments (`SET <key> <value>`), rejecting excess tokens or unsupported options with standard Redis syntax error.

### Line 158: `        std::string key(tokens[1]);`
### Line 159: `        std::string val(tokens[2]);`
Constructs owned strings from argument views.

### Line 160: `        db[std::move(key)] = std::move(val);`
Stores key and value in database using move semantics.

### Line 161: `        write_buf += "+OK\r\n";`
Appends simple string response `+OK\r\n` directly to output buffer.

### Line 162: `        return;`
Exits execution after handling `SET`.

### Line 163: `    }`
Closes `SET` handler.

### Line 164: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 165: `    if (equals_ci(cmd, "GET")) {`
Checks for `GET` command case-insensitively.

### Line 166: `        if (tokens.size() != 2) {`
### Line 167: `            write_buf += "-ERR wrong number of arguments for 'get' command\r\n";`
### Line 168: `            return;`
### Line 169: `        }`
Validates argument count for `GET`.

### Line 170: `        auto it = db.find(std::string(tokens[1]));`
Looks up key in database hash map in $O(1)$ amortized time.

### Line 171: `        if (it != db.end()) {`
### Line 172: `            write_buf += "$" + std::to_string(it->second.size()) + "\r\n";`
### Line 173: `            write_buf += it->second;`
### Line 174: `            write_buf += "\r\n";`
Appends bulk string response `$<len>\r\n<value>\r\n` if key exists.

### Line 175: `        } else {`
### Line 176: `            write_buf += "$-1\r\n";`
Appends null bulk string `$-1\r\n` if key is not found.

### Line 177: `        }`
Closes found vs not-found branching.

### Line 178: `        return;`
Exits execution after handling `GET`.

### Line 179: `    }`
Closes `GET` handler.

### Line 180: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 181: `    if (equals_ci(cmd, "DEL")) {`
Checks for `DEL` command case-insensitively.

### Line 182: `        if (tokens.size() < 2) {`
### Line 183: `            write_buf += "-ERR wrong number of arguments for 'del' command\r\n";`
### Line 184: `            return;`
### Line 185: `        }`
Validates that at least one key argument is provided.

### Line 186: `        int count = 0;`
### Line 187: `        for (size_t i = 1; i < tokens.size(); ++i) {`
### Line 188: `            count += db.erase(std::string(tokens[i]));`
### Line 189: `        }`
Erases each specified key from database, accumulating count of erased keys.

### Line 190: `        write_buf += ":" + std::to_string(count) + "\r\n";`
Appends integer response `:<count>\r\n` directly to output buffer.

### Line 191: `        return;`
Exits execution after handling `DEL`.

### Line 192: `    }`
Closes `DEL` handler.

### Line 193: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 194: `    if (equals_ci(cmd, "COMMAND")) {`
### Line 195: `        write_buf += "*0\r\n";`
Returns empty array `*0\r\n` for `redis-cli` handshake compatibility.

### Line 196: `        return;`
### Line 197: `    }`
Closes `COMMAND` handler.

### Line 198: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 199: `    if (equals_ci(cmd, "INFO")) {`
### Line 200: `        write_buf += "$0\r\n\r\n";`
Returns empty bulk string `$0\r\n\r\n` for `INFO` compatibility.

### Line 201: `        return;`
### Line 202: `    }`
Closes `INFO` handler.

### Line 203: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 204: `    if (equals_ci(cmd, "QUIT")) {`
### Line 205: `        should_close = true;`
### Line 206: `        write_buf += "+OK\r\n";`
Sets `should_close = true` to trigger teardown after flushing and appends `+OK\r\n`.

### Line 207: `        return;`
### Line 208: `    }`
Closes `QUIT` handler.

### Line 209: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 210: `    if (equals_ci(cmd, "HELLO")) {`
### Line 211: `        write_buf += "-ERR unknown command 'HELLO'\r\n";`
Rejects `HELLO` command for RESP2 downgrade negotiation.

### Line 212: `        return;`
### Line 213: `    }`
Closes `HELLO` handler.

### Line 214: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 215: `    write_buf += "-ERR unknown command '";`
### Line 216: `    write_buf.append(cmd.data(), cmd.size());`
### Line 217: `    write_buf += "'\r\n";`
Serializes standard Redis unknown command error `-ERR unknown command '<cmd>'\r\n`.

### Line 218: `}`
Closes `execute_tokens()`.

### Line 219: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 220: `ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close) {`
Main dual-mode parsing and execution entry point.

### Line 221: `    if (cursor >= buf.size()) {`
### Line 222: `        return ParseResult::NEED_MORE_DATA;`
### Line 223: `    }`
Returns `NEED_MORE_DATA` if cursor has reached end of buffer.

### Line 224: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 225: `    if (buf[cursor] == '*') {`
Branch 1: Dispatches to RESP Multibulk Array parser if buffer starts with `*`.

### Line 226: `        size_t consumed = 0;`
### Line 227: `        std::vector<std::string_view> args;`
### Line 228: `        bool protocol_error = false;`
### Line 229: `        bool success = parse_resp_multibulk(buf, cursor, consumed, args, protocol_error);`
Executes zero-copy multibulk parsing.

### Line 230: `        if (protocol_error) {`
### Line 231: `            cursor += consumed;`
### Line 232: `            should_close = true;`
### Line 233: `            write_buf += "-ERR Protocol error: invalid multibulk length\r\n";`
### Line 234: `            return ParseResult::SUCCESS;`
### Line 235: `        }`
Handles fatal protocol violations (e.g. invalid length or oversized multibulk counts) by responding with standard protocol error, setting `should_close = true`, and advancing cursor.

### Line 236: `        if (!success) {`
### Line 237: `            return ParseResult::NEED_MORE_DATA;`
### Line 238: `        }`
Returns `NEED_MORE_DATA` if partial multibulk frame is awaiting network arrival.

### Line 239: `        cursor += consumed;`
Advances cursor past parsed multibulk command frame.

### Line 240: `        execute_tokens(args, write_buf, should_close);`
Executes extracted multibulk command tokens.

### Line 241: `        return ParseResult::SUCCESS;`
Returns `SUCCESS` indicating complete command execution.

### Line 242: `    }`
Closes multibulk array branch.

### Line 243: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 244: `    size_t pos = buf.find('\n', cursor);`
### Line 245: `    if (pos == std::string::npos) {`
### Line 246: `        return ParseResult::NEED_MORE_DATA;`
### Line 247: `    }`
Branch 2: Inline command framing. Locates newline `\n`; returns `NEED_MORE_DATA` if incomplete line.

### Line 248: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 249: `    std::string_view line(buf.data() + cursor, pos - cursor);`
Constructs string view over candidate line.

### Line 250: `    cursor = pos + 1;`
Advances cursor past newline character.

### Line 251: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 252: `    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {`
### Line 253: `        line.remove_suffix(1);`
### Line 254: `    }`
Trims trailing carriage returns and whitespace.

### Line 255: `    while (!line.empty() && line.front() == ' ') {`
### Line 256: `        line.remove_prefix(1);`
### Line 257: `    }`
Trims leading whitespace.

### Line 258: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 259: `    if (line.empty()) {`
### Line 260: `        return ParseResult::SUCCESS;`
### Line 261: `    }`
Ignores empty blank lines, returning `SUCCESS`.

### Line 262: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 263: `    auto inline_tokens = tokenize_inline(line);`
Tokenizes inline command string with quote and escape awareness.

### Line 264: `    if (inline_tokens.empty()) {`
### Line 265: `        return ParseResult::SUCCESS;`
### Line 266: `    }`
Returns `SUCCESS` if no tokens were parsed.

### Line 267: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 268: `    std::vector<std::string_view> view_tokens;`
### Line 269: `    view_tokens.reserve(inline_tokens.size());`
### Line 270: `    for (const auto &tok : inline_tokens) {`
### Line 271: `        view_tokens.emplace_back(tok);`
### Line 272: `    }`
Converts token strings into `std::string_view` array matching `execute_tokens` interface.

### Line 273: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 274: `    execute_tokens(view_tokens, write_buf, should_close);`
Executes tokenized inline command.

### Line 275: `    return ParseResult::SUCCESS;`
Returns `SUCCESS`.

### Line 276: `}`
Closes `parse_and_execute()`.

### Line 277: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 278: `}`
Closes `namespace Protocol`.
