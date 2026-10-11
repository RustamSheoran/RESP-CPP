# Documentation: `src/protocol.cpp`

The [`src/protocol.cpp`](file:///home/rustam/Projects/resp-cpp/src/protocol.cpp) file implements the dual-mode Redis command parser, quote-aware tokenizer, in-memory key-value database, zero-copy multibulk parsing, direct egress buffer appending, and command execution routines.

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
Returns `false` upon the first character mismatch.

### Line 17: `        }`
Closes the character mismatch check block.

### Line 18: `    }`
Closes the character comparison loop.

### Line 19: `    return true;`
Returns `true` when all characters match case-insensitively.

### Line 20: `}`
Closes `equals_ci`.

### Line 21: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 22: `static std::vector<std::string> tokenize_inline(std::string_view raw_cmd) {`
Defines quote-aware tokenizer converting inline commands into a vector of arguments.

### Line 23: `    std::vector<std::string> tokens;`
Initializes vector accumulating extracted argument tokens.

### Line 24: `    size_t i = 0;`
Sets scanning pointer to start of command string.

### Line 25: `    while (i < raw_cmd.size()) {`
Loops through the command string until end of input.

### Line 26: `        while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {`
### Line 27: `            i++;`
### Line 28: `        }`
Advances pointer past leading whitespace characters.

### Line 29: `        if (i >= raw_cmd.size()) break;`
Exits loop if end of input is reached after skipping whitespace.

### Line 30: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 31: `        if (raw_cmd[i] == '"' || raw_cmd[i] == '\'') {`
Checks if the current token begins with a double quote (`"`) or single quote (`'`).

### Line 32: `            char quote = raw_cmd[i];`
Records opening quote character for matching.

### Line 33: `            i++;`
Advances pointer past the opening quote.

### Line 34: `            size_t start = i;`
Records start position of the quoted content.

### Line 35: `            while (i < raw_cmd.size() && raw_cmd[i] != quote) {`
Scans characters until the matching closing quote delimiter is reached.

### Line 36: `                if (raw_cmd[i] == '\\' && i + 1 < raw_cmd.size()) {`
### Line 37: `                    i += 2;`
### Line 38: `                } else {`
### Line 39: `                    i++;`
### Line 40: `                }`
Handles backslash escape sequences (`\"`, `\'`, `\\`), skipping escaped characters.

### Line 41: `            }`
Closes quoted scanning loop.

### Line 42: `            std::string tok;`
Allocates string for unescaped token content.

### Line 43: `            for (size_t k = start; k < i; ++k) {`
Iterates over quoted slice to resolve escape sequences.

### Line 44: `                if (raw_cmd[k] == '\\' && k + 1 < i) {`
### Line 45: `                    tok.push_back(raw_cmd[k + 1]);`
### Line 46: `                    k++;`
### Line 47: `                } else {`
### Line 48: `                    tok.push_back(raw_cmd[k]);`
### Line 49: `                }`
Appends literal characters, stripping escaping backslashes.

### Line 50: `            }`
Closes escape resolution loop.

### Line 51: `            tokens.push_back(std::move(tok));`
Pushes the unquoted token string into `tokens`.

### Line 52: `            if (i < raw_cmd.size() && raw_cmd[i] == quote) {`
### Line 53: `                i++;`
### Line 54: `            }`
Advances past the closing quote character.

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
Zero-copy RESP Multibulk Array parser extracting arguments as `std::string_view` directly into `args` without heap allocations.

### Line 67: `    protocol_error = false;`
Initializes protocol error flag.

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
Parses array argument count via `std::stoll`.

### Line 83: `    if (num_args <= 0) {`
### Line 84: `        consumed = line_end + 2 - cursor;`
### Line 85: `        return true;`
### Line 86: `    }`
Handles empty array commands.

### Line 87: `    size_t pos = line_end + 2;`
Positions pointer at start of first bulk string argument.

### Line 88: `    std::vector<std::string_view> parsed;`
### Line 89: `    parsed.reserve(num_args);`
Allocates and reserves vector of string views.

### Line 90: `    for (long long i = 0; i < num_args; ++i) {`
Iterates through all array arguments.

### Line 91: `        if (pos >= buf.size() || buf[pos] != '$') {`
### Line 92: `            return false;`
### Line 93: `        }`
Validates `$` bulk string prefix.

### Line 94: `        size_t bulk_line_end = buf.find("\r\n", pos);`
### Line 95: `        if (bulk_line_end == std::string::npos) {`
### Line 96: `            return false;`
### Line 97: `        }`
Locates length terminator `\r\n`.

### Line 98: `        long long bulk_len = 0;`
### Line 99: `        try {`
### Line 100: `            bulk_len = std::stoll(buf.substr(pos + 1, bulk_line_end - (pos + 1)));`
### Line 101: `        } catch (...) {`
### Line 102: `            protocol_error = true;`
### Line 103: `            consumed = bulk_line_end + 2 - cursor;`
### Line 104: `            return false;`
### Line 105: `        }`
Parses bulk string byte length.

### Line 106: `        if (bulk_len < 0) {`
### Line 107: `            return false;`
### Line 108: `        }`
Rejects invalid negative length.

### Line 109: `        pos = bulk_line_end + 2;`
Advances pointer to bulk payload start.

### Line 110: `        if (pos + static_cast<size_t>(bulk_len) + 2 > buf.size()) {`
### Line 111: `            return false;`
### Line 112: `        }`
Checks that the entire payload plus framing `\r\n` has arrived. Returns false (need more data) if incomplete.

### Line 113: `        if (buf[pos + bulk_len] != '\r' || buf[pos + bulk_len + 1] != '\n') {`
### Line 114: `            return false;`
### Line 115: `        }`
Verifies framing delimiter bytes.

### Line 116: `        parsed.emplace_back(buf.data() + pos, bulk_len);`
Creates a zero-copy string view referencing payload data without copying memory.

### Line 117: `        pos += bulk_len + 2;`
Advances pointer past payload and trailing `\r\n`.

### Line 118: `    }`
Closes argument loop.

### Line 119: `    consumed = pos - cursor;`
Calculates total bytes consumed from `cursor`.

### Line 120: `    args = std::move(parsed);`
Moves argument views vector to output.

### Line 121: `    return true;`
Returns true indicating success.

### Line 122: `}`
Closes `parse_resp_multibulk`.

### Line 123: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 124: `static void execute_tokens(const std::vector<std::string_view> &tokens, std::string &write_buf, bool &should_close) {`
Executes command tokens and appends serialized responses directly into `write_buf` without intermediate allocations.

### Line 125: `    if (tokens.empty()) {`
### Line 126: `        return;`
### Line 127: `    }`
Returns if token list is empty.

### Line 128: `    std::string_view cmd = tokens[0];`
Extracts command verb.

### Line 129: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 130: `    if (equals_ci(cmd, "PING")) {`
Handles `PING` command.

### Line 131: `        if (tokens.size() == 1) {`
### Line 132: `            write_buf += "+PONG\r\n";`
Directly appends `+PONG\r\n` to socket write buffer with zero string allocations.

### Line 133: `        } else if (tokens.size() == 2) {`
### Line 134: `            std::string_view msg = tokens[1];`
### Line 135: `            write_buf += "$" + std::to_string(msg.size()) + "\r\n";`
### Line 136: `            write_buf.append(msg.data(), msg.size());`
### Line 137: `            write_buf += "\r\n";`
Appends bulk string echo response directly to write buffer.

### Line 138: `        } else {`
### Line 139: `            write_buf += "-ERR wrong number of arguments for 'ping' command\r\n";`
### Line 140: `        }`
Appends error on extra arguments.

### Line 141: `        return;`
### Line 142: `    }`
Exits `PING` handler.

### Line 143: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 144: `    if (equals_ci(cmd, "SET")) {`
Handles `SET` command.

### Line 145: `        if (tokens.size() < 3) {`
### Line 146: `            write_buf += "-ERR wrong number of arguments for 'set' command\r\n";`
### Line 147: `            return;`
### Line 148: `        }`
Validates argument count.

### Line 149: `        std::string key(tokens[1]);`
### Line 150: `        std::string val(tokens[2]);`
Extracts key and value strings.

### Line 151: `        if (tokens.size() > 3) {`
### Line 152: `            for (size_t i = 3; i < tokens.size(); ++i) {`
### Line 153: `                val += " ";`
### Line 154: `                val.append(tokens[i].data(), tokens[i].size());`
### Line 155: `            }`
### Line 156: `        }`
Concatenates multi-word unquoted inline arguments.

### Line 157: `        db[std::move(key)] = std::move(val);`
Moves key-value pair into hash map.

### Line 158: `        write_buf += "+OK\r\n";`
Appends `+OK\r\n` directly to write buffer.

### Line 159: `        return;`
### Line 160: `    }`
Exits `SET` handler.

### Line 161: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 162: `    if (equals_ci(cmd, "GET")) {`
Handles `GET` command.

### Line 163: `        if (tokens.size() != 2) {`
### Line 164: `            write_buf += "-ERR wrong number of arguments for 'get' command\r\n";`
### Line 165: `            return;`
### Line 166: `        }`
Validates key argument.

### Line 167: `        auto it = db.find(std::string(tokens[1]));`
Looks up key in hash map.

### Line 168: `        if (it != db.end()) {`
### Line 169: `            write_buf += "$" + std::to_string(it->second.size()) + "\r\n";`
### Line 170: `            write_buf += it->second;`
### Line 171: `            write_buf += "\r\n";`
Appends bulk string value directly to write buffer.

### Line 172: `        } else {`
### Line 173: `            write_buf += "$-1\r\n";`
Appends null bulk string reply if not found.

### Line 174: `        }`
Closes key found check.

### Line 175: `        return;`
### Line 176: `    }`
Exits `GET` handler.

### Line 177: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 178: `    if (equals_ci(cmd, "DEL")) {`
Handles `DEL` command.

### Line 179: `        if (tokens.size() < 2) {`
### Line 180: `            write_buf += "-ERR wrong number of arguments for 'del' command\r\n";`
### Line 181: `            return;`
### Line 182: `        }`
Validates argument count.

### Line 183: `        int count = 0;`
### Line 184: `        for (size_t i = 1; i < tokens.size(); ++i) {`
### Line 185: `            count += db.erase(std::string(tokens[i]));`
### Line 186: `        }`
Erases keys from `db` and tallies count.

### Line 187: `        write_buf += ":" + std::to_string(count) + "\r\n";`
Appends integer reply directly to write buffer.

### Line 188: `        return;`
### Line 189: `    }`
Exits `DEL` handler.

### Line 190: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 191: `    if (equals_ci(cmd, "COMMAND")) {`
### Line 192: `        write_buf += "*0\r\n";`
### Line 193: `        return;`
### Line 194: `    }`
Appends empty array for command handshake.

### Line 195: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 196: `    if (equals_ci(cmd, "INFO")) {`
### Line 197: `        write_buf += "$0\r\n\r\n";`
### Line 198: `        return;`
### Line 199: `    }`
Appends empty bulk string for server info.

### Line 200: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 201: `    if (equals_ci(cmd, "QUIT")) {`
### Line 202: `        should_close = true;`
### Line 203: `        write_buf += "+OK\r\n";`
### Line 204: `        return;`
### Line 205: `    }`
Sets teardown flag and appends `+OK\r\n`.

### Line 206: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 207: `    if (equals_ci(cmd, "HELLO")) {`
### Line 208: `        write_buf += "-ERR unknown command 'HELLO'\r\n";`
### Line 209: `        return;`
### Line 210: `    }`
Rejects RESP3 HELLO command, prompting fallback to RESP2.

### Line 211: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 212: `    write_buf += "-ERR unknown command '";`
### Line 213: `    write_buf.append(cmd.data(), cmd.size());`
### Line 214: `    write_buf += "'\r\n";`
Appends formatted error for unrecognized verbs.

### Line 215: `}`
Closes `execute_tokens`.

### Line 216: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 217: `ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close) {`
Main streaming entry point using batch cursor tracking.

### Line 218: `    if (cursor >= buf.size()) {`
### Line 219: `        return ParseResult::NEED_MORE_DATA;`
### Line 220: `    }`
Returns if cursor reaches end of buffer.

### Line 221: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 222: `    if (buf[cursor] == '*') {`
Branch executing multibulk parser.

### Line 223: `        size_t consumed = 0;`
### Line 224: `        std::vector<std::string_view> args;`
### Line 225: `        bool protocol_error = false;`
### Line 226: `        bool success = parse_resp_multibulk(buf, cursor, consumed, args, protocol_error);`
Parses multibulk array at `cursor`.

### Line 227: `        if (protocol_error) {`
### Line 228: `            cursor += consumed;`
### Line 229: `            write_buf += "-ERR Protocol error: invalid multibulk length\r\n";`
### Line 230: `            return ParseResult::SUCCESS;`
### Line 231: `        }`
Handles multibulk protocol errors.

### Line 232: `        if (!success) {`
### Line 233: `            return ParseResult::NEED_MORE_DATA;`
### Line 234: `        }`
Returns `NEED_MORE_DATA` without modifying cursor if array is incomplete.

### Line 235: `        cursor += consumed;`
Advances cursor past parsed command bytes (zero `memmove` calls).

### Line 236: `        execute_tokens(args, write_buf, should_close);`
Executes command directly appending into `write_buf`.

### Line 237: `        return ParseResult::SUCCESS;`
### Line 238: `    }`
Ends multibulk branch.

### Line 239: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 240: `    size_t pos = buf.find('\n', cursor);`
### Line 241: `    if (pos == std::string::npos) {`
### Line 242: `        return ParseResult::NEED_MORE_DATA;`
### Line 243: `    }`
Searches for newline delimiter `\n` starting at `cursor`.

### Line 244: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 245: `    std::string_view line(buf.data() + cursor, pos - cursor);`
### Line 246: `    cursor = pos + 1;`
Slices inline command line view and advances cursor past `\n`.

### Line 247: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 248: `    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {`
### Line 249: `        line.remove_suffix(1);`
### Line 250: `    }`
Trims trailing carriage returns and whitespace.

### Line 251: `    while (!line.empty() && line.front() == ' ') {`
### Line 252: `        line.remove_prefix(1);`
### Line 253: `    }`
Trims leading spaces.

### Line 254: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 255: `    if (line.empty()) {`
### Line 256: `        return ParseResult::SUCCESS;`
### Line 257: `    }`
Ignores empty lines.

### Line 258: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 259: `    auto inline_tokens = tokenize_inline(line);`
### Line 260: `    if (inline_tokens.empty()) {`
### Line 261: `        return ParseResult::SUCCESS;`
### Line 262: `    }`
Tokenizes inline command.

### Line 263: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 264: `    std::vector<std::string_view> view_tokens;`
### Line 265: `    view_tokens.reserve(inline_tokens.size());`
### Line 266: `    for (const auto &tok : inline_tokens) {`
### Line 267: `        view_tokens.emplace_back(tok);`
### Line 268: `    }`
Builds view tokens list.

### Line 269: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 270: `    execute_tokens(view_tokens, write_buf, should_close);`
Executes command directly appending into `write_buf`.

### Line 271: `    return ParseResult::SUCCESS;`
Returns success.

### Line 272: `}`
Closes `parse_and_execute`.

### Line 273: *(Empty Line)*
Whitespace separator for visual clarity.

### Line 274: `}`
Closes `namespace Protocol`.
