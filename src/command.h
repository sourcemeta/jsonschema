#ifndef SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_
#define SOURCEMETA_JSONSCHEMA_CLI_COMMAND_H_

#include <sourcemeta/core/options.h>

namespace sourcemeta::jsonschema {
auto fmt(const sourcemeta::core::Options &options) -> void;
auto inspect(const sourcemeta::core::Options &options) -> void;
auto bundle(const sourcemeta::core::Options &options) -> void;
auto test(const sourcemeta::core::Options &options) -> void;
auto lint(const sourcemeta::core::Options &options) -> void;
auto validate(const sourcemeta::core::Options &options) -> void;
auto metaschema(const sourcemeta::core::Options &options) -> void;
auto compile(const sourcemeta::core::Options &options) -> void;
auto encode(const sourcemeta::core::Options &options) -> void;
auto decode(const sourcemeta::core::Options &options) -> void;
} // namespace sourcemeta::jsonschema

#endif
