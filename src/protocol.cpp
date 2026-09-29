#include "protocol.h"
#include <unordered_map>
#include <vector>
#include <cctype>

namespace Protocol {

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

    if (equals_ci(cmd, "SET")) {
        if (tokens.size() < 3) {
            return "-ERR wrong number of arguments for 'set' command\r\n";
        }
        std::string key(tokens[1]);
        std::string val(tokens[2]);

        if (tokens.size() > 3) {
            size_t val_start = tokens[2].data() - raw_cmd.data();
            val = std::string(raw_cmd.substr(val_start));
            while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) {
                val.pop_back();
            }
        }

        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        db[key] = val;
        return "+OK\r\n";
    }

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

    if (equals_ci(cmd, "COMMAND")) {
        return "*0\r\n";
    }
    if (equals_ci(cmd, "QUIT")) {
        return "+OK\r\n";
    }

    return "-ERR unknown command '" + std::string(cmd) + "'\r\n";
}

}
