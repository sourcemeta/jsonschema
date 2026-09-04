#ifndef SOURCEMETA_JSONSCHEMA_CLI_LOGGER_H_
#define SOURCEMETA_JSONSCHEMA_CLI_LOGGER_H_

#include <sourcemeta/core/options.h>

#include <fstream>  // std::ofstream
#include <iostream> // std::cerr
#include <ostream>  // std::ostream

namespace sourcemeta::jsonschema {

// These loggers are spelled in upper case to read as the macros that
// conditional logging conventionally takes the shape of at the call site
// NOLINTBEGIN(readability-identifier-naming)

inline auto LOG_VERBOSE(const sourcemeta::core::Options &options)
    -> std::ostream & {
  if (options.contains("verbose") || options.contains("debug")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

inline auto LOG_DEBUG(const sourcemeta::core::Options &options)
    -> std::ostream & {
  if (options.contains("debug")) {
    std::cerr << "debug: ";
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

inline auto LOG_WARNING() -> std::ostream & {
  std::cerr << "warning: ";
  return std::cerr;
}

// NOLINTEND(readability-identifier-naming)

} // namespace sourcemeta::jsonschema

#endif
