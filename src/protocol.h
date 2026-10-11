#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

namespace Protocol {

enum class ParseResult {
    SUCCESS,
    NEED_MORE_DATA
};

ParseResult parse_and_execute(const std::string &buf, size_t &cursor, std::string &write_buf, bool &should_close);

}

#endif
