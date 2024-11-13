#ifndef SOURCEMETA_HYDRA_HTTPSERVER_LOGGER_H
#define SOURCEMETA_HYDRA_HTTPSERVER_LOGGER_H

#ifndef SOURCEMETA_HYDRA_HTTPSERVER_EXPORT
#include <sourcemeta/hydra/httpserver_export.h>
#endif

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::hydra::http {

/// @ingroup httpserver
/// This class serves as the foundation for coordinated, thread-safe logging
/// machinery in the HTTP server. Different instances of this logger (with
/// different identifiers) will be created for global and per-request logging
class SOURCEMETA_HYDRA_HTTPSERVER_EXPORT ServerLogger {
public:
// These constructors are considered private. Do not use them directly.
#if !defined(DOXYGEN)
  ServerLogger();
  ServerLogger(std::string &&id);
#endif

  /// Retrieve the unique identifier of the logging instance. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <iostream>
  ///
  /// // Assuming you got a logger instance called `logger`
  /// std::cout << logger.id() << "\n";
  /// ```
  auto id() const -> std::string_view;

  /// Log a message. This class only ensures thread-safety for individual
  /// messages. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <iostream>
  ///
  /// // Assuming you got a logger instance called `logger`
  /// logger << "Hello World!";
  /// ```
  auto operator<<(std::string_view message) const -> void;

private:
  const std::string identifier;
};

} // namespace sourcemeta::hydra::http

#endif
