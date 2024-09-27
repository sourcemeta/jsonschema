#ifndef SOURCEMETA_JSONBINPACK_CLI_COMMANDS_H_
#define SOURCEMETA_JSONBINPACK_CLI_COMMANDS_H_

#include <filesystem> // std::filesystem
#include <string>     // std::string

namespace sourcemeta::jsonbinpack::cli {
auto help(const std::string &program) -> int;
auto version() -> int;
auto canonicalize(const std::filesystem::path &schema_path) -> int;
auto canonicalize() -> int;
auto compile(const std::filesystem::path &schema_path) -> int;
auto compile() -> int;
auto encode(const std::filesystem::path &schema_path,
            const std::filesystem::path &instance_path) -> int;
auto encode(const std::filesystem::path &schema_path) -> int;
auto decode(const std::filesystem::path &schema_path,
            const std::filesystem::path &data_path) -> int;
auto decode(const std::filesystem::path &schema_path) -> int;
} // namespace sourcemeta::jsonbinpack::cli

#endif
