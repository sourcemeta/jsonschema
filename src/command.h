#ifndef SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_
#define SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_

#include <span>   // std::span
#include <string> // std::string

namespace sourcemeta::jsonschema::cli {
auto fmt(const std::span<const std::string> &arguments) -> int;
auto inspect(const std::span<const std::string> &arguments) -> int;
auto bundle(const std::span<const std::string> &arguments) -> int;
auto test(const std::span<const std::string> &arguments) -> int;
auto lint(const std::span<const std::string> &arguments) -> int;
auto validate(const std::span<const std::string> &arguments) -> int;
auto metaschema(const std::span<const std::string> &arguments) -> int;
auto encode(const std::span<const std::string> &arguments) -> int;
auto decode(const std::span<const std::string> &arguments) -> int;
} // namespace sourcemeta::jsonschema::cli

#endif
