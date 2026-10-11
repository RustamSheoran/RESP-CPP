#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

namespace Protocol {

constexpr long long MAX_MULTIBULK_ARGS = 1024 * 1024;
constexpr long long MAX_BULK_LEN = 512 * 1024 * 1024;

enum class ParseResult {
    SUCCESS,
    NEED_MORE_DATA
};

ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close);

}

#endif
