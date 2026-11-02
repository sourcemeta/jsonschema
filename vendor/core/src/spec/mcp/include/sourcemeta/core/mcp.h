#ifndef SOURCEMETA_CORE_MCP_H_
#define SOURCEMETA_CORE_MCP_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonrpc.h>
#include <sourcemeta/core/mcp_capabilities.h>
#include <sourcemeta/core/mcp_error.h>
#include <sourcemeta/core/mcp_headers.h>
#include <sourcemeta/core/mcp_logging.h>
#include <sourcemeta/core/mcp_protocol.h>
#include <sourcemeta/core/mcp_results.h>
// NOLINTEND(misc-include-cleaner)

/// @defgroup mcp MCP
/// @brief Helpers for building Model Context Protocol (MCP) envelopes.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// ```
///
/// @par Deployment boundaries
/// Enforce transport byte limits, decompression budgets and timeouts in the
/// application. MCP envelope validation does not limit the memory or time
/// already spent receiving and parsing a message.
/// After parsing, sourcemeta::core::mcp_validate_request_meta checks envelope
/// metadata, not the caller's identity or application authorization.
///
/// Authenticate the caller and authorize each resource before opening a file or
/// making a network request. URI validation and
/// sourcemeta::core::mcp_make_resource do not grant resource-access permission.
///
/// sourcemeta::core::mcp_validate_continuation checks protocol fields and,
/// optionally, an exact state-token echo. The application must protect state
/// integrity, bind it to the caller and operation, and enforce expiry and
/// replay policy. These are application controls, not additional requirements
/// imposed by Core's MCP validators. Multiple workers need shared atomic state
/// consumption.

#endif
