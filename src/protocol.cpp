#include "protocol.h"
#include <unordered_map>
#include <vector>
#include <string_view>
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

static std::vector<std::string> tokenize_inline(std::string_view raw_cmd) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < raw_cmd.size()) {
        while (i < raw_cmd.size() && (raw_cmd[i] == ' ' || raw_cmd[i] == '\t' || raw_cmd[i] == '\r')) {
            i++;
        }
        if (i >= raw_cmd.size()) break;

        if (raw_cmd[i] == '"' || raw_cmd[i] == '\'') {
            char quote = raw_cmd[i];
            i++;
            size_t start = i;
            while (i < raw_cmd.size() && raw_cmd[i] != quote) {
                if (raw_cmd[i] == '\\' && i + 1 < raw_cmd.size()) {
                    i += 2;
                } else {
                    i++;
                }
            }
            std::string tok;
            for (size_t k = start; k < i; ++k) {
                if (raw_cmd[k] == '\\' && k + 1 < i) {
                    tok.push_back(raw_cmd[k + 1]);
                    k++;
                } else {
                    tok.push_back(raw_cmd[k]);
                }
            }
            tokens.push_back(std::move(tok));
            if (i < raw_cmd.size() && raw_cmd[i] == quote) {
                i++;
            }
        } else {
            size_t start = i;
            while (i < raw_cmd.size() && raw_cmd[i] != ' ' && raw_cmd[i] != '\t' && raw_cmd[i] != '\r') {
                i++;
            }
            tokens.emplace_back(raw_cmd.substr(start, i - start));
        }
    }
    return tokens;
}

static bool parse_resp_multibulk(const std::string &buf, size_t cursor, size_t &consumed, std::vector<std::string_view> &args, bool &protocol_error) {
    protocol_error = false;
    if (cursor >= buf.size() || buf[cursor] != '*') {
        return false;
    }
    size_t line_end = buf.find("\r\n", cursor);
    if (line_end == std::string::npos) {
        return false;
    }
    long long num_args = 0;
    try {
        num_args = std::stoll(buf.substr(cursor + 1, line_end - (cursor + 1)));
    } catch (...) {
        protocol_error = true;
        consumed = line_end + 2 - cursor;
        return false;
    }
    if (num_args <= 0) {
        consumed = line_end + 2 - cursor;
        return true;
    }
    size_t pos = line_end + 2;
    std::vector<std::string_view> parsed;
    parsed.reserve(num_args);
    for (long long i = 0; i < num_args; ++i) {
        if (pos >= buf.size() || buf[pos] != '$') {
            return false;
        }
        size_t bulk_line_end = buf.find("\r\n", pos);
        if (bulk_line_end == std::string::npos) {
            return false;
        }
        long long bulk_len = 0;
        try {
            bulk_len = std::stoll(buf.substr(pos + 1, bulk_line_end - (pos + 1)));
        } catch (...) {
            protocol_error = true;
            consumed = bulk_line_end + 2 - cursor;
            return false;
        }
        if (bulk_len < 0) {
            return false;
        }
        pos = bulk_line_end + 2;
        if (pos + static_cast<size_t>(bulk_len) + 2 > buf.size()) {
            return false;
        }
        if (buf[pos + bulk_len] != '\r' || buf[pos + bulk_len + 1] != '\n') {
            return false;
        }
        parsed.emplace_back(buf.data() + pos, bulk_len);
        pos += bulk_len + 2;
    }
    consumed = pos - cursor;
    args = std::move(parsed);
    return true;
}

static void execute_tokens(const std::vector<std::string_view> &tokens, std::string &write_buf, bool &should_close) {
    if (tokens.empty()) {
        return;
    }
    std::string_view cmd = tokens[0];

    if (equals_ci(cmd, "PING")) {
        if (tokens.size() == 1) {
            write_buf += "+PONG\r\n";
        } else if (tokens.size() == 2) {
            std::string_view msg = tokens[1];
            write_buf += "$" + std::to_string(msg.size()) + "\r\n";
            write_buf.append(msg.data(), msg.size());
            write_buf += "\r\n";
        } else {
            write_buf += "-ERR wrong number of arguments for 'ping' command\r\n";
        }
        return;
    }

    if (equals_ci(cmd, "SET")) {
        if (tokens.size() < 3) {
            write_buf += "-ERR wrong number of arguments for 'set' command\r\n";
            return;
        }
        std::string key(tokens[1]);
        std::string val(tokens[2]);
        if (tokens.size() > 3) {
            for (size_t i = 3; i < tokens.size(); ++i) {
                val += " ";
                val.append(tokens[i].data(), tokens[i].size());
            }
        }
        db[std::move(key)] = std::move(val);
        write_buf += "+OK\r\n";
        return;
    }

    if (equals_ci(cmd, "GET")) {
        if (tokens.size() != 2) {
            write_buf += "-ERR wrong number of arguments for 'get' command\r\n";
            return;
        }
        auto it = db.find(std::string(tokens[1]));
        if (it != db.end()) {
            write_buf += "$" + std::to_string(it->second.size()) + "\r\n";
            write_buf += it->second;
            write_buf += "\r\n";
        } else {
            write_buf += "$-1\r\n";
        }
        return;
    }

    if (equals_ci(cmd, "DEL")) {
        if (tokens.size() < 2) {
            write_buf += "-ERR wrong number of arguments for 'del' command\r\n";
            return;
        }
        int count = 0;
        for (size_t i = 1; i < tokens.size(); ++i) {
            count += db.erase(std::string(tokens[i]));
        }
        write_buf += ":" + std::to_string(count) + "\r\n";
        return;
    }

    if (equals_ci(cmd, "COMMAND")) {
        write_buf += "*0\r\n";
        return;
    }

    if (equals_ci(cmd, "INFO")) {
        write_buf += "$0\r\n\r\n";
        return;
    }

    if (equals_ci(cmd, "QUIT")) {
        should_close = true;
        write_buf += "+OK\r\n";
        return;
    }

    if (equals_ci(cmd, "HELLO")) {
        write_buf += "-ERR unknown command 'HELLO'\r\n";
        return;
    }

    write_buf += "-ERR unknown command '";
    write_buf.append(cmd.data(), cmd.size());
    write_buf += "'\r\n";
}

ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close) {
    if (cursor >= buf.size()) {
        return ParseResult::NEED_MORE_DATA;
    }

    if (buf[cursor] == '*') {
        size_t consumed = 0;
        std::vector<std::string_view> args;
        bool protocol_error = false;
        bool success = parse_resp_multibulk(buf, cursor, consumed, args, protocol_error);
        if (protocol_error) {
            cursor += consumed;
            write_buf += "-ERR Protocol error: invalid multibulk length\r\n";
            return ParseResult::SUCCESS;
        }
        if (!success) {
            return ParseResult::NEED_MORE_DATA;
        }
        cursor += consumed;
        execute_tokens(args, write_buf, should_close);
        return ParseResult::SUCCESS;
    }

    size_t pos = buf.find('\n', cursor);
    if (pos == std::string::npos) {
        return ParseResult::NEED_MORE_DATA;
    }

    std::string_view line(buf.data() + cursor, pos - cursor);
    cursor = pos + 1;

    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
        line.remove_suffix(1);
    }
    while (!line.empty() && line.front() == ' ') {
        line.remove_prefix(1);
    }

    if (line.empty()) {
        return ParseResult::SUCCESS;
    }

    auto inline_tokens = tokenize_inline(line);
    if (inline_tokens.empty()) {
        return ParseResult::SUCCESS;
    }

    std::vector<std::string_view> view_tokens;
    view_tokens.reserve(inline_tokens.size());
    for (const auto &tok : inline_tokens) {
        view_tokens.emplace_back(tok);
    }

    execute_tokens(view_tokens, write_buf, should_close);
    return ParseResult::SUCCESS;
}

}
