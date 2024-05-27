#ifndef INTELLIGENCE_JSONSCHEMA_CLI_COMMAND_H_
#define INTELLIGENCE_JSONSCHEMA_CLI_COMMAND_H_

#include <span>   // std::span
#include <string> // std::string

namespace intelligence::jsonschema::cli {
auto help(const std::string &program) -> int;
auto fmt(const std::span<const std::string> &arguments) -> int;
auto frame(const std::span<const std::string> &arguments) -> int;
auto lint(const std::span<const std::string> &arguments) -> int;
auto validate(const std::span<const std::string> &arguments) -> int;
} // namespace intelligence::jsonschema::cli

#endif
