#include "protocol.h"
#include <unordered_map>
#include <vector>
#include <cctype>

namespace Protocol {

// In-memory key-value database
static std::unordered_map<std::string, std::string> db;

static bool equals_ci(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(a[i])) !=
            std::toupper(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

static std::vector<std::string_view> tokenize(std::string_view raw_cmd) {
    std::vector<std::string_view> tokens;
    size_t i = 0;
    while (i < raw_cmd.size()) {
        // Skip leading whitespace and carriage returns
        while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {
            i++;
        }
        if (i >= raw_cmd.size()) break;

        size_t start = i;
        while (i < raw_cmd.size() && raw_cmd[i] != ' ' && raw_cmd[i] != '\t' && raw_cmd[i] != '\r') {
            i++;
        }
        tokens.push_back(raw_cmd.substr(start, i - start));
    }
    return tokens;
}

std::string execute_command(std::string_view raw_cmd) {
    std::vector<std::string_view> tokens = tokenize(raw_cmd);
    if (tokens.empty()) {
        return "";
    }

    std::string_view cmd = tokens[0];

    // 1. PING -> +PONG\r\n or $len\r\n<msg>\r\n
    if (equals_ci(cmd, "PING")) {
        if (tokens.size() == 1) {
            return "+PONG\r\n";
        } else if (tokens.size() == 2) {
            std::string_view msg = tokens[1];
            return "$" + std::to_string(msg.size()) + "\r\n" + std::string(msg) + "\r\n";
        } else {
            return "-ERR wrong number of arguments for 'ping' command\r\n";
        }
    }

    // 2. SET <key> <val> -> +OK\r\n
    if (equals_ci(cmd, "SET")) {
        if (tokens.size() < 3) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        std::string key(tokens[1]);
        std::string val(tokens[2]);

        // Support values with spaces if passed without quotes: SET key word1 word2 ...
        if (tokens.size() > 3) {
            size_t val_start = tokens[2].data() - raw_cmd.data();
            val = std::string(raw_cmd.substr(val_start));
            while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) {
                val.pop_back();
            }
        }

        // Strip surrounding quotes if present: "value" -> value
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        // Store into in-memory hash map (ownership transfer from view to string)
        db[key] = val;
        return "+OK\r\n";
    }

    // 3. GET <key> -> $len\r\n<val>\r\n or $-1\r\n (nil)
    if (equals_ci(cmd, "GET")) {
        if (tokens.size() != 2) {
            return "-ERR wrong number of arguments for 'get' command\r\n";
        }
        std::string key(tokens[1]);
        auto it = db.find(key);
        if (it != db.end()) {
            return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
        }
        return "$-1\r\n";
    }

    // 4. DEL <key> [<key2> ...] -> :<count>\r\n
    if (equals_ci(cmd, "DEL")) {
        if (tokens.size() < 2) {
            return "-ERR wrong number of arguments for 'del' command\r\n";
        }
        int count = 0;
        for (size_t i = 1; i < tokens.size(); ++i) {
            count += db.erase(std::string(tokens[i]));
        }
        return ":" + std::to_string(count) + "\r\n";
    }

    // Redis-cli handshake & session commands compatibility
    if (equals_ci(cmd, "COMMAND")) {
        return "*0\r\n";
    }
    if (equals_ci(cmd, "QUIT")) {
        return "+OK\r\n";
    }

    return "-ERR unknown command '" + std::string(cmd) + "'\r\n";
}

} // namespace Protocol
