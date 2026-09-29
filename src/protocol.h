#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <string_view>

namespace Protocol {

std::string execute_command(std::string_view raw_cmd);

}

#endif
