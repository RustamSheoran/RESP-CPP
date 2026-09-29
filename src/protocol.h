#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <string_view>

namespace Protocol {

// Takes a view into the command buffer (ZERO heap allocations during parsing!)
std::string execute_command(std::string_view raw_cmd);

} // namespace Protocol

#endif // PROTOCOL_H
