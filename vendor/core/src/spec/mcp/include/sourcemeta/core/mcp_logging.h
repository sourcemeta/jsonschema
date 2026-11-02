#ifndef SOURCEMETA_CORE_MCP_LOGGING_H_
#define SOURCEMETA_CORE_MCP_LOGGING_H_

#include <sourcemeta/core/json.h>

#include <array>    // std::array
#include <cstdint>  // std::uint8_t
#include <optional> // std::optional, std::nullopt
#include <utility>  // std::unreachable

namespace sourcemeta::core {

/// @ingroup mcp
/// Syslog severity levels, ordered from least to most severe. A request's
/// logging threshold admits its own level and all more severe levels. The
/// enumerator order is the severity order, so the levels compare directly.
///
/// This feature is deprecated as of protocol version `2026-07-28`, which
/// states that new implementations should not adopt it and that existing
/// implementations should migrate to standard error output for the stdio
/// transport, or to a general-purpose observability framework.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/logging
/// https://modelcontextprotocol.io/specification/2026-07-28/deprecated
enum class MCPLogLevel : std::uint8_t {
  /// Detailed information useful for debugging.
  Debug,
  /// General informational messages.
  Info,
  /// Normal but significant conditions.
  Notice,
  /// Conditions that may require attention.
  Warning,
  /// Error conditions.
  Error,
  /// Critical conditions.
  Critical,
  /// Conditions requiring immediate action.
  Alert,
  /// Conditions indicating that the system is unusable.
  Emergency
};

/// @ingroup mcp
/// Convert a logging severity to its canonical wire string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_log_level_string(
///            sourcemeta::core::MCPLogLevel::Warning) == "warning");
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/logging
[[nodiscard]] constexpr auto
mcp_log_level_string(const MCPLogLevel level) noexcept -> JSON::StringView {
  switch (level) {
    case MCPLogLevel::Debug:
      return "debug";
    case MCPLogLevel::Info:
      return "info";
    case MCPLogLevel::Notice:
      return "notice";
    case MCPLogLevel::Warning:
      return "warning";
    case MCPLogLevel::Error:
      return "error";
    case MCPLogLevel::Critical:
      return "critical";
    case MCPLogLevel::Alert:
      return "alert";
    case MCPLogLevel::Emergency:
      return "emergency";
  }
    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

/// @ingroup mcp
/// Parse an exact, case-sensitive logging severity without allocating. An
/// unrecognized value yields no level, which the specification asks a server
/// to report as an invalid parameter. For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_resolve_log_level("notice") ==
///        sourcemeta::core::MCPLogLevel::Notice);
/// assert(!sourcemeta::core::mcp_resolve_log_level("NOTICE").has_value());
/// assert(!sourcemeta::core::mcp_resolve_log_level("verbose").has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/logging
[[nodiscard]] constexpr auto
mcp_resolve_log_level(const JSON::StringView value) noexcept
    -> std::optional<MCPLogLevel> {
  constexpr std::array<MCPLogLevel, 8> LEVELS{
      {MCPLogLevel::Debug, MCPLogLevel::Info, MCPLogLevel::Notice,
       MCPLogLevel::Warning, MCPLogLevel::Error, MCPLogLevel::Critical,
       MCPLogLevel::Alert, MCPLogLevel::Emergency}};
  for (const auto level : LEVELS) {
    if (mcp_log_level_string(level) == value) {
      return level;
    }
  }
  return std::nullopt;
}

/// @ingroup mcp
/// Whether a notification meets an explicitly selected logging threshold. A
/// threshold admits its own severity and every more severe one. The
/// specification scopes a threshold to the request that carried it and only
/// permits a notification once a request has asked for one, so a caller
/// reaches this predicate having already found a threshold to apply. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_log_level_enabled(
///     sourcemeta::core::MCPLogLevel::Error,
///     sourcemeta::core::MCPLogLevel::Warning));
/// assert(!sourcemeta::core::mcp_log_level_enabled(
///     sourcemeta::core::MCPLogLevel::Debug,
///     sourcemeta::core::MCPLogLevel::Warning));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/logging
[[nodiscard]] constexpr auto
mcp_log_level_enabled(const MCPLogLevel level,
                      const MCPLogLevel threshold) noexcept -> bool {
  return level >= threshold;
}

} // namespace sourcemeta::core
#endif
