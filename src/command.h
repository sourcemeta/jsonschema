#ifndef SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_
#define SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_

#include <sourcemeta/core/options.h>

namespace sourcemeta::jsonschema::cli {
auto fmt(const sourcemeta::core::Options &options) -> int;
auto inspect(const sourcemeta::core::Options &options) -> int;
auto bundle(const sourcemeta::core::Options &options) -> int;
auto test(const sourcemeta::core::Options &options) -> int;
auto lint(const sourcemeta::core::Options &options) -> int;
auto validate(const sourcemeta::core::Options &options) -> int;
auto metaschema(const sourcemeta::core::Options &options) -> int;
auto compile(const sourcemeta::core::Options &options) -> int;
auto encode(const sourcemeta::core::Options &options) -> int;
auto decode(const sourcemeta::core::Options &options) -> int;
} // namespace sourcemeta::jsonschema::cli

#endif
