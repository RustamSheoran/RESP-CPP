# Documentation: `src/protocol.cpp`

The [`src/protocol.cpp`](file:///home/rustam/Projects/resp-cpp/src/protocol.cpp) file implements the Redis serialization protocol (RESP) command parser, in-memory dictionary storage, zero-copy tokenizer, and Redis command execution routines.

---

## Complete Source Code

```cpp
1: #include "protocol.h"
2: #include <unordered_map>
3: #include <vector>
4: #include <cctype>
5: 
6: namespace Protocol {
7: 
8: static std::unordered_map<std::string, std::string> db;
9: 
10: static bool equals_ci(std::string_view a, std::string_view b) {
11:     if (a.size() != b.size()) return false;
12:     for (size_t i = 0; i < a.size(); ++i) {
13:         if (std::toupper(static_cast<unsigned char>(a[i])) !=
14:             std::toupper(static_cast<unsigned char>(b[i]))) {
15:             return false;
16:         }
17:     }
18:     return true;
19: }
20: 
21: static std::vector<std::string_view> tokenize(std::string_view raw_cmd) {
22:     std::vector<std::string_view> tokens;
23:     size_t i = 0;
24:     while (i < raw_cmd.size()) {
25:         while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {
26:             i++;
27:         }
28:         if (i >= raw_cmd.size()) break;
29: 
30:         size_t start = i;
31:         while (i < raw_cmd.size() && raw_cmd[i] != ' ' && raw_cmd[i] != '\t' && raw_cmd[i] != '\r') {
32:             i++;
33:         }
34:         tokens.push_back(raw_cmd.substr(start, i - start));
35:     }
36:     return tokens;
37: }
38: 
39: std::string execute_command(std::string_view raw_cmd) {
40:     std::vector<std::string_view> tokens = tokenize(raw_cmd);
41:     if (tokens.empty()) {
42:         return "";
43:     }
44: 
45:     std::string_view cmd = tokens[0];
46: 
47:     if (equals_ci(cmd, "PING")) {
48:         if (tokens.size() == 1) {
49:             return "+PONG\r\n";
50:         } else if (tokens.size() == 2) {
51:             std::string_view msg = tokens[1];
52:             return "$" + std::to_string(msg.size()) + "\r\n" + std::string(msg) + "\r\n";
53:         } else {
54:             return "-ERR wrong number of arguments for 'ping' command\r\n";
55:         }
56:     }
57: 
58:     if (equals_ci(cmd, "SET")) {
59:         if (tokens.size() < 3) {
60:             return "-ERR wrong number of arguments for 'set' command\r\n";
61:         }
62:         std::string key(tokens[1]);
63:         std::string val(tokens[2]);
64: 
65:         if (tokens.size() > 3) {
66:             size_t val_start = tokens[2].data() - raw_cmd.data();
67:             val = std::string(raw_cmd.substr(val_start));
68:             while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) {
69:                 val.pop_back();
70:             }
71:         }
72: 
73:         if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
74:             val = val.substr(1, val.size() - 2);
75:         }
76: 
77:         db[key] = val;
78:         return "+OK\r\n";
79:     }
80: 
81:     if (equals_ci(cmd, "GET")) {
82:         if (tokens.size() != 2) {
83:             return "-ERR wrong number of arguments for 'get' command\r\n";
84:         }
85:         std::string key(tokens[1]);
86:         auto it = db.find(key);
87:         if (it != db.end()) {
88:             return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
89:         }
90:         return "$-1\r\n";
91:     }
92: 
93:     if (equals_ci(cmd, "DEL")) {
94:         if (tokens.size() < 2) {
95:             return "-ERR wrong number of arguments for 'del' command\r\n";
96:         }
97:         int count = 0;
98:         for (size_t i = 1; i < tokens.size(); ++i) {
99:             count += db.erase(std::string(tokens[i]));
100:         }
101:         return ":" + std::to_string(count) + "\r\n";
102:     }
103: 
104:     if (equals_ci(cmd, "COMMAND")) {
105:         return "*0\r\n";
106:     }
107:     if (equals_ci(cmd, "QUIT")) {
108:         return "+OK\r\n";
109:     }
110: 
111:     return "-ERR unknown command '" + std::string(cmd) + "'\r\n";
112: }
113: 
114: }
```

---

## Line-by-Line Breakdown & Explanation

### Includes & Namespace (Lines 1–6)
- **Line 1: `#include "protocol.h"`**  
  Includes the interface header to ensure implementation signatures match declarations.
- **Line 2: `#include <unordered_map>`**  
  Provides `std::unordered_map` for the in-memory key-value dictionary, offering amortized $O(1)$ key lookup and insertion.
- **Line 3: `#include <vector>`**  
  Provides dynamic array container `std::vector` used to store tokens parsed from the command line.
- **Line 4: `#include <cctype>`**  
  Provides `std::toupper` for case-insensitive character comparisons.
- **Line 5: *(Empty Line)***  
  Visual formatting separation.
- **Line 6: `namespace Protocol {`**  
  Encloses all protocol handling within the `Protocol` namespace.

---

### In-Memory Database (Line 8)
- **Line 8: `static std::unordered_map<std::string, std::string> db;`**  
  - `static`: Internal linkage within this translation unit, isolating `db` from outside tampering.
  - `std::unordered_map<std::string, std::string>`: The actual in-memory Redis database storing string keys mapped to string values.
  - Guarantees $O(1)$ average complexity for `SET`, `GET`, and `DEL` operations.

---

### Case-Insensitive String Comparison (Lines 10–19)
- **Line 10: `static bool equals_ci(std::string_view a, std::string_view b) {`**  
  Declares a static helper function to compare two `std::string_view` instances case-insensitively (e.g. `ping`, `PING`, and `pInG` are equivalent).
- **Line 11: `    if (a.size() != b.size()) return false;`**  
  Fast-path rejection: If the string lengths differ, they cannot be equal.
- **Line 12: `    for (size_t i = 0; i < a.size(); ++i) {`**  
  Iterates through every character of both string views simultaneously.
- **Line 13–14: `        if (std::toupper(static_cast<unsigned char>(a[i])) != std::toupper(static_cast<unsigned char>(b[i]))) {`**  
  Converts each character to unsigned char and compares their uppercase values using `std::toupper`.
- **Line 15: `            return false;`**  
  Returns `false` upon the first character mismatch.
- **Line 16: `        }`**  
  Closes the `if` block.
- **Line 17: `    }`**  
  Closes the loop.
- **Line 18: `    return true;`**  
  Returns `true` if all characters matched.
- **Line 19: `}`**  
  Closes `equals_ci`.

---

### Zero-Copy Tokenizer (Lines 21–37)
- **Line 21: `static std::vector<std::string_view> tokenize(std::string_view raw_cmd) {`**  
  Slices a command line into constituent word tokens without copying or allocating strings on the heap.
- **Line 22: `    std::vector<std::string_view> tokens;`**  
  Initializes a vector to collect token slices.
- **Line 23: `    size_t i = 0;`**  
  Index pointer starting at the beginning of the command string view.
- **Line 24: `    while (i < raw_cmd.size()) {`**  
  Iterates until the entire string view is traversed.
- **Line 25: `        while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {`**  
  Advances past leading whitespace characters (spaces, tabs, carriage returns).
- **Line 26: `            i++;`**  
  Increments index pointer.
- **Line 27: `        }`**  
  Closes leading whitespace loop.
- **Line 28: `        if (i >= raw_cmd.size()) break;`**  
  If the end of the string view is reached after skipping whitespace, breaks out.
- **Line 30: `        size_t start = i;`**  
  Records the start index of a non-whitespace token.
- **Line 31: `        while (i < raw_cmd.size() && raw_cmd[i] != ' ' && raw_cmd[i] != '\t' && raw_cmd[i] != '\r') {`**  
  Consumes non-whitespace characters comprising the token.
- **Line 32: `            i++;`**  
  Increments index pointer.
- **Line 33: `        }`**  
  Closes token consumption loop.
- **Line 34: `        tokens.push_back(raw_cmd.substr(start, i - start));`**  
  Slices a sub-view using `substr(start, length)` and pushes it into `tokens`. This does zero heap allocations.
- **Line 35: `    }`**  
  Closes outer tokenization loop.
- **Line 36: `    return tokens;`**  
  Returns the vector of token string views.
- **Line 37: `}`**  
  Closes `tokenize`.

---

### Command Dispatcher & Execution (Lines 39–112)
- **Line 39: `std::string execute_command(std::string_view raw_cmd) {`**  
  Primary command execution entry point.
- **Line 40: `    std::vector<std::string_view> tokens = tokenize(raw_cmd);`**  
  Tokenizes the command line view.
- **Line 41: `    if (tokens.empty()) {`**  
  Checks if the incoming line had no tokens (empty or whitespace only).
- **Line 42: `        return "";`**  
  Returns empty string, resulting in no response sent for empty lines.
- **Line 43: `    }`**  
  Closes empty check.
- **Line 45: `    std::string_view cmd = tokens[0];`**  
  Extracts the first token as the command verb.

#### 1. PING Command (Lines 47–56)
- **Line 47: `    if (equals_ci(cmd, "PING")) {`**  
  Checks if command is `PING`.
- **Line 48: `        if (tokens.size() == 1) {`**  
  If no arguments supplied:
- **Line 49: `            return "+PONG\r\n";`**  
  Returns standard RESP simple string `+PONG\r\n`.
- **Line 50: `        } else if (tokens.size() == 2) {`**  
  If an echo argument is provided (e.g., `PING "hello"`):
- **Line 51: `            std::string_view msg = tokens[1];`**  
  Extracts message token view.
- **Line 52: `            return "$" + std::to_string(msg.size()) + "\r\n" + std::string(msg) + "\r\n";`**  
  Returns standard RESP bulk string with byte length header and payload.
- **Line 53–55: `        } else { return "-ERR wrong number of arguments for 'ping' command\r\n"; }`**  
  Returns RESP error if more than 1 argument passed.
- **Line 56: `    }`**  
  Closes `PING` handler.

#### 2. SET Command (Lines 58–79)
- **Line 58: `    if (equals_ci(cmd, "SET")) {`**  
  Checks if command is `SET`.
- **Line 59–61: `        if (tokens.size() < 3) { return "-ERR wrong number of arguments for 'set' command\r\n"; }`**  
  Validates that at least key and value are provided.
- **Line 62: `        std::string key(tokens[1]);`**  
  Constructs `key` string from token view.
- **Line 63: `        std::string val(tokens[2]);`**  
  Constructs initial `val` string.
- **Line 65–71:**  
  Handles multi-word values without quotes (e.g. `SET msg hello world`):
  - Line 66: Computes pointer offset of value start relative to `raw_cmd`.
  - Line 67: Extracts the entire remainder of `raw_cmd` as `val`.
  - Line 68–70: Trims trailing carriage returns (`\r`) and spaces.
- **Line 73–75:**  
  Handles quoted values (e.g. `SET msg "hello world"`):
  - Slices off enclosing quotation marks if length $\ge 2$ and starts/ends with `"`.
- **Line 77: `        db[key] = val;`**  
  Inserts or updates the key-value pair in `std::unordered_map`.
- **Line 78: `        return "+OK\r\n";`**  
  Returns standard RESP simple string `+OK\r\n`.
- **Line 79: `    }`**  
  Closes `SET` handler.

#### 3. GET Command (Lines 81–91)
- **Line 81: `    if (equals_ci(cmd, "GET")) {`**  
  Checks if command is `GET`.
- **Line 82–84: `        if (tokens.size() != 2) { return "-ERR wrong number of arguments for 'get' command\r\n"; }`**  
  Validates exactly one argument (`key`) provided.
- **Line 85: `        std::string key(tokens[1]);`**  
  Creates key string to look up.
- **Line 86: `        auto it = db.find(key);`**  
  Searches hash map in $O(1)$ average time.
- **Line 87–89: `        if (it != db.end()) { return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n"; }`**  
  If key exists, formats as RESP bulk string `$<length>\r\n<value>\r\n`.
- **Line 90: `        return "$-1\r\n";`**  
  If key does not exist, returns RESP null bulk string `$-1\r\n` (nil).
- **Line 91: `    }`**  
  Closes `GET` handler.

#### 4. DEL Command (Lines 93–102)
- **Line 93: `    if (equals_ci(cmd, "DEL")) {`**  
  Checks if command is `DEL`.
- **Line 94–96: `        if (tokens.size() < 2) { return "-ERR wrong number of arguments for 'del' command\r\n"; }`**  
  Validates at least one key argument provided.
- **Line 97: `        int count = 0;`**  
  Initializes count of successfully deleted keys.
- **Line 98–100: `        for (size_t i = 1; i < tokens.size(); ++i) { count += db.erase(std::string(tokens[i])); }`**  
  Iterates over all requested keys; `db.erase()` returns 1 if key existed, 0 otherwise.
- **Line 101: `        return ":" + std::to_string(count) + "\r\n";`**  
  Returns RESP integer format `:<count>\r\n`.
- **Line 102: `    }`**  
  Closes `DEL` handler.

#### 5. Compatibility Commands (Lines 104–109)
- **Line 104–106: `    if (equals_ci(cmd, "COMMAND")) { return "*0\r\n"; }`**  
  Responds to Redis CLI/driver handshake with empty array `*0\r\n`.
- **Line 107–109: `    if (equals_ci(cmd, "QUIT")) { return "+OK\r\n"; }`**  
  Responds to client exit request with `+OK\r\n`.

#### 6. Unknown Command Handling (Lines 111–114)
- **Line 111: `    return "-ERR unknown command '" + std::string(cmd) + "'\r\n";`**  
  Formats and returns standard RESP error for unhandled commands.
- **Line 112: `}`**  
  Closes `execute_command`.
- **Line 114: `}`**  
  Closes `namespace Protocol`.
