#ifndef SOURCEMETA_CORE_MCP_PROTOCOL_H_
#define SOURCEMETA_CORE_MCP_PROTOCOL_H_

#ifndef SOURCEMETA_CORE_MCP_EXPORT
#include <sourcemeta/core/mcp_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <cstdint>  // std::uint8_t
#include <optional> // std::optional, std::nullopt
#include <utility>  // std::to_underlying, std::unreachable

namespace sourcemeta::core {

/// @ingroup mcp
/// The supported MCP protocol revisions.
///
/// MCP 2026-07-28 is a major architectural redesign replacing stateful
/// initialization with stateless, per-request capability negotiation, removing
/// `initialize`, `ping`, and `logging/setLevel`, and introducing
/// `server/discover`, `subscriptions/listen`, explicit result types, and
/// caching policies.
///
/// Supported legacy revisions (2025-03-26 through 2025-11-25) use
/// connection-scoped initialization handshakes and stateful protocol sessions.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
enum class MCPProtocolVersion : std::uint8_t {
  /// Protocol revision 2025-03-26.
  V_2025_03_26,
  /// Protocol revision 2025-06-18.
  V_2025_06_18,
  /// The initialization-handshake revision 2025-11-25.
  V_2025_11_25,
  /// The stateless, per-request capability negotiation revision (2026-07-28).
  V_2026_07_28,
};

/// @ingroup mcp
/// Wire string constant for MCP protocol version `2025-03-26`.
constexpr JSON::StringView MCP_PROTOCOL_VERSION_2025_03_26{"2025-03-26"};

/// @ingroup mcp
/// Wire string constant for MCP protocol version `2025-06-18`.
constexpr JSON::StringView MCP_PROTOCOL_VERSION_2025_06_18{"2025-06-18"};

/// @ingroup mcp
/// Wire string constant for MCP protocol version `2025-11-25`.
constexpr JSON::StringView MCP_PROTOCOL_VERSION_2025_11_25{"2025-11-25"};

/// @ingroup mcp
/// Wire string constant for MCP protocol version `2026-07-28`.
constexpr JSON::StringView MCP_PROTOCOL_VERSION_2026_07_28{"2026-07-28"};

/// @ingroup mcp
/// Wire string for the initialization handshake request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#initializerequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#initializerequest
constexpr JSON::StringView MCP_METHOD_INITIALIZE{"initialize"};

/// @ingroup mcp
/// Wire string for the initialization confirmation notification.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#initializednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#initializednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_INITIALIZED{
    "notifications/initialized"};

/// @ingroup mcp
/// Wire string for the connection liveness check method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#pingrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#pingrequest
constexpr JSON::StringView MCP_METHOD_PING{"ping"};

/// @ingroup mcp
/// Wire string for the 2026-07-28 server discovery request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#discoverrequest
constexpr JSON::StringView MCP_METHOD_SERVER_DISCOVER{"server/discover"};

/// @ingroup mcp
/// Wire string for the 2026-07-28 subscriptions listen request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#subscriptionslistenrequest
constexpr JSON::StringView MCP_METHOD_SUBSCRIPTIONS_LISTEN{
    "subscriptions/listen"};

/// @ingroup mcp
/// Wire string for the 2026-07-28 subscriptions acknowledged notification.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#subscriptionsacknowledgednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_SUBSCRIPTIONS_ACKNOWLEDGED{
    "notifications/subscriptions/acknowledged"};

/// @ingroup mcp
/// Wire string for the tools list request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#listtoolsrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listtoolsrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#listtoolsrequest
constexpr JSON::StringView MCP_METHOD_TOOLS_LIST{"tools/list"};

/// @ingroup mcp
/// Wire string for the tool call request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#calltoolrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#calltoolrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#calltoolrequest
constexpr JSON::StringView MCP_METHOD_TOOLS_CALL{"tools/call"};

/// @ingroup mcp
/// Wire string for the resources list request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#listresourcesrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listresourcesrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#listresourcesrequest
constexpr JSON::StringView MCP_METHOD_RESOURCES_LIST{"resources/list"};

/// @ingroup mcp
/// Wire string for the resource read request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#readresourcerequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#readresourcerequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#readresourcerequest
constexpr JSON::StringView MCP_METHOD_RESOURCES_READ{"resources/read"};

/// @ingroup mcp
/// Wire string for the resource subscribe request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#subscriberequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#subscriberequest
constexpr JSON::StringView MCP_METHOD_RESOURCES_SUBSCRIBE{
    "resources/subscribe"};

/// @ingroup mcp
/// Wire string for the resource unsubscribe request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#unsubscriberequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#unsubscriberequest
constexpr JSON::StringView MCP_METHOD_RESOURCES_UNSUBSCRIBE{
    "resources/unsubscribe"};

/// @ingroup mcp
/// Wire string for the resource templates list request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#listresourcetemplatesrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listresourcetemplatesrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#listresourcetemplatesrequest
constexpr JSON::StringView MCP_METHOD_RESOURCES_TEMPLATES_LIST{
    "resources/templates/list"};

/// @ingroup mcp
/// Wire string for the prompts list request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#listpromptsrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listpromptsrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#listpromptsrequest
constexpr JSON::StringView MCP_METHOD_PROMPTS_LIST{"prompts/list"};

/// @ingroup mcp
/// Wire string for the prompt get request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#getpromptrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#getpromptrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#getpromptrequest
constexpr JSON::StringView MCP_METHOD_PROMPTS_GET{"prompts/get"};

/// @ingroup mcp
/// Wire string for the completion request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#completerequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#completerequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#completerequest
constexpr JSON::StringView MCP_METHOD_COMPLETION_COMPLETE{
    "completion/complete"};

/// @ingroup mcp
/// Wire string for the legacy logging set-level request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#setlevelrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#setlevelrequest
constexpr JSON::StringView MCP_METHOD_LOGGING_SET_LEVEL{"logging/setLevel"};

/// @ingroup mcp
/// Wire string for the cancelled notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#cancellednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#cancellednotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#cancellednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_CANCELLED{
    "notifications/cancelled"};

/// @ingroup mcp
/// Wire string for the message notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#loggingmessagenotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#loggingmessagenotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#loggingmessagenotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_MESSAGE{
    "notifications/message"};

/// @ingroup mcp
/// Wire string for the progress notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#progressnotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#progressnotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#progressnotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_PROGRESS{
    "notifications/progress"};

/// @ingroup mcp
/// Wire string for the prompts list changed notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#promptlistchangednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#promptlistchangednotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#promptlistchangednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED{
    "notifications/prompts/list_changed"};

/// @ingroup mcp
/// Wire string for the resources list changed notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#resourcelistchangednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#resourcelistchangednotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#resourcelistchangednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED{
    "notifications/resources/list_changed"};

/// @ingroup mcp
/// Wire string for the resources updated notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#resourceupdatednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#resourceupdatednotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#resourceupdatednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED{
    "notifications/resources/updated"};

/// @ingroup mcp
/// Wire string for the tools list changed notification method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#toollistchangednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#toollistchangednotification
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#toollistchangednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED{
    "notifications/tools/list_changed"};

/// @ingroup mcp
/// Wire string for the roots list request method.
///
/// This feature is deprecated as of protocol version `2026-07-28`, which
/// states that new implementations should not adopt it. The method remains
/// here because the library still speaks the revisions that define it.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/deprecated
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#listrootsrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listrootsrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#listrootsrequest
constexpr JSON::StringView MCP_METHOD_ROOTS_LIST{"roots/list"};

/// @ingroup mcp
/// Wire string for the sampling create-message request method.
///
/// This feature is deprecated as of protocol version `2026-07-28`, which
/// states that new implementations should not adopt it. The method remains
/// here because the library still speaks the revisions that define it.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/deprecated
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#createmessagerequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#createmessagerequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#createmessagerequest
constexpr JSON::StringView MCP_METHOD_SAMPLING_CREATE_MESSAGE{
    "sampling/createMessage"};

/// @ingroup mcp
/// Wire string for the elicitation create request method.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#elicitrequest
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#elicitrequest
/// https://modelcontextprotocol.io/specification/2026-07-28/schema#elicitrequest
constexpr JSON::StringView MCP_METHOD_ELICITATION_CREATE{"elicitation/create"};

/// @ingroup mcp
/// Wire method name for `notifications/elicitation/complete`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#elicitationcompletenotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_ELICITATION_COMPLETE{
    "notifications/elicitation/complete"};

/// @ingroup mcp
/// Wire method name for `notifications/roots/list_changed`.
///
/// This feature is deprecated as of protocol version `2026-07-28`, which
/// states that new implementations should not adopt it. The method remains
/// here because the library still speaks the revisions that define it.
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/deprecated
/// https://modelcontextprotocol.io/specification/2025-06-18/schema#rootslistchangednotification
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#rootslistchangednotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED{
    "notifications/roots/list_changed"};

/// @ingroup mcp
/// Wire method name for `notifications/tasks/status`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#taskstatusnotification
constexpr JSON::StringView MCP_METHOD_NOTIFICATIONS_TASKS_STATUS{
    "notifications/tasks/status"};

/// @ingroup mcp
/// Wire method name for `tasks/cancel`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#canceltaskrequest
constexpr JSON::StringView MCP_METHOD_TASKS_CANCEL{"tasks/cancel"};

/// @ingroup mcp
/// Wire method name for `tasks/get`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#gettaskrequest
constexpr JSON::StringView MCP_METHOD_TASKS_GET{"tasks/get"};

/// @ingroup mcp
/// Wire method name for `tasks/list`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#listtasksrequest
constexpr JSON::StringView MCP_METHOD_TASKS_LIST{"tasks/list"};

/// @ingroup mcp
/// Wire method name for `tasks/result`.
///
/// @see
/// https://modelcontextprotocol.io/specification/2025-11-25/schema#gettaskpayloadrequest
constexpr JSON::StringView MCP_METHOD_TASKS_RESULT{"tasks/result"};

/// @ingroup mcp
/// Convert an @ref MCPProtocolVersion enum value to its canonical string
/// representation.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_protocol_version_string(
///            sourcemeta::core::MCPProtocolVersion::V_2026_07_28) ==
///        "2026-07-28");
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning
constexpr auto
mcp_protocol_version_string(const MCPProtocolVersion version) noexcept
    -> JSON::StringView {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
      return MCP_PROTOCOL_VERSION_2025_03_26;
    case MCPProtocolVersion::V_2025_06_18:
      return MCP_PROTOCOL_VERSION_2025_06_18;
    case MCPProtocolVersion::V_2025_11_25:
      return MCP_PROTOCOL_VERSION_2025_11_25;
    case MCPProtocolVersion::V_2026_07_28:
      return MCP_PROTOCOL_VERSION_2026_07_28;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Resolve an MCP protocol version wire string to its corresponding enum value,
/// or `std::nullopt` when the string does not match any recognized version.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_resolve_protocol_version("2025-11-25") ==
///        sourcemeta::core::MCPProtocolVersion::V_2025_11_25);
/// assert(!sourcemeta::core::mcp_resolve_protocol_version("1999-01-01")
///             .has_value());
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning
constexpr auto
mcp_resolve_protocol_version(const JSON::StringView value) noexcept
    -> std::optional<MCPProtocolVersion> {
  if (value == MCP_PROTOCOL_VERSION_2026_07_28) {
    return MCPProtocolVersion::V_2026_07_28;
  }
  if (value == MCP_PROTOCOL_VERSION_2025_11_25) {
    return MCPProtocolVersion::V_2025_11_25;
  }
  if (value == MCP_PROTOCOL_VERSION_2025_06_18) {
    return MCPProtocolVersion::V_2025_06_18;
  }
  if (value == MCP_PROTOCOL_VERSION_2025_03_26) {
    return MCPProtocolVersion::V_2025_03_26;
  }
  return std::nullopt;
}

/// @ingroup mcp
/// Check whether an MCP protocol revision is at least the given minimum
/// version.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_protocol_version_at_least(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// assert(!sourcemeta::core::mcp_protocol_version_at_least(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26,
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning
constexpr auto
mcp_protocol_version_at_least(const MCPProtocolVersion current,
                              const MCPProtocolVersion minimum) noexcept
    -> bool {
  return std::to_underlying(current) >= std::to_underlying(minimum);
}

/// @ingroup mcp
/// Check whether the given wire string is a valid, recognized MCP protocol
/// version.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_protocol_version_is_valid("2026-07-28"));
/// assert(!sourcemeta::core::mcp_protocol_version_is_valid("1999-01-01"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning
constexpr auto
mcp_protocol_version_is_valid(const JSON::StringView value) noexcept -> bool {
  return mcp_resolve_protocol_version(value).has_value();
}

/// @ingroup mcp
/// Whether the specified MCP protocol version uses an initialization handshake
/// (`initialize` / `notifications/initialized`).
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_uses_initialization_handshake(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// assert(!sourcemeta::core::mcp_uses_initialization_handshake(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/lifecycle
constexpr auto
mcp_uses_initialization_handshake(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports the `ping` method.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_ping(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// assert(!sourcemeta::core::mcp_supports_ping(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/lifecycle
constexpr auto mcp_supports_ping(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether requests in the specified protocol version must carry metadata
/// inside `params._meta`.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_requires_request_meta(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_requires_request_meta(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
constexpr auto
mcp_requires_request_meta(const MCPProtocolVersion version) noexcept -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether result responses in the specified protocol version must carry an
/// explicit `resultType` field.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_requires_result_type(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_requires_result_type(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_requires_result_type(const MCPProtocolVersion version) noexcept -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether cacheable responses in the specified protocol version must include
/// `ttlMs` and `cacheScope`.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_requires_cacheable_metadata(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_requires_cacheable_metadata(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
constexpr auto
mcp_requires_cacheable_metadata(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports the 2026-07-28
/// `server/discover` method.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_server_discover(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_supports_server_discover(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/discover
constexpr auto
mcp_supports_server_discover(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports 2026-07-28 server-side
/// subscriptions via `subscriptions/listen`.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_subscriptions_listen(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_supports_subscriptions_listen(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
constexpr auto
mcp_supports_subscriptions_listen(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports Multi Round-Trip
/// Results (`input_required`).
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_mrtr(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// assert(!sourcemeta::core::mcp_supports_mrtr(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
constexpr auto mcp_supports_mrtr(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports stateful protocol
/// sessions.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_protocol_sessions(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// assert(!sourcemeta::core::mcp_supports_protocol_sessions(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http
constexpr auto
mcp_supports_protocol_sessions(const MCPProtocolVersion version) noexcept
    -> bool {
  return version != MCPProtocolVersion::V_2026_07_28;
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports output schema on tool
/// definitions.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_output_schema(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// assert(!sourcemeta::core::mcp_supports_output_schema(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools
constexpr auto
mcp_supports_output_schema(const MCPProtocolVersion version) noexcept -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_06_18);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports structured content.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_structured_content(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// assert(!sourcemeta::core::mcp_supports_structured_content(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/tools
constexpr auto
mcp_supports_structured_content(const MCPProtocolVersion version) noexcept
    -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_06_18);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports resource link content.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_resource_link_content(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// assert(!sourcemeta::core::mcp_supports_resource_link_content(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_supports_resource_link_content(const MCPProtocolVersion version) noexcept
    -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_06_18);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports implementation title.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_implementation_title(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// assert(!sourcemeta::core::mcp_supports_implementation_title(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_supports_implementation_title(const MCPProtocolVersion version) noexcept
    -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_06_18);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports implementation
/// description.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_implementation_description(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// assert(!sourcemeta::core::mcp_supports_implementation_description(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto mcp_supports_implementation_description(
    const MCPProtocolVersion version) noexcept -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_11_25);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports implementation
/// website URL.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_implementation_website_url(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25));
/// assert(!sourcemeta::core::mcp_supports_implementation_website_url(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto mcp_supports_implementation_website_url(
    const MCPProtocolVersion version) noexcept -> bool {
  return mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_11_25);
}

/// @ingroup mcp
/// Whether the specified MCP protocol version supports JSON-RPC batching.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_jsonrpc_batching(
///     sourcemeta::core::MCPProtocolVersion::V_2025_03_26));
/// assert(!sourcemeta::core::mcp_supports_jsonrpc_batching(
///     sourcemeta::core::MCPProtocolVersion::V_2025_06_18));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http
constexpr auto
mcp_supports_jsonrpc_batching(const MCPProtocolVersion version) noexcept
    -> bool {
  return version == MCPProtocolVersion::V_2025_03_26;
}

/// @ingroup mcp
/// Whether a base client-to-server request is defined in this revision.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "tools/call"));
/// assert(!sourcemeta::core::mcp_is_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "ping"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto mcp_is_request_method(const MCPProtocolVersion version,
                                     const JSON::StringView method) noexcept
    -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
    case MCPProtocolVersion::V_2025_06_18:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_INITIALIZE ||
             method == MCP_METHOD_LOGGING_SET_LEVEL ||
             method == MCP_METHOD_PING || method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_SUBSCRIBE ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_RESOURCES_UNSUBSCRIBE ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
    case MCPProtocolVersion::V_2025_11_25:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_INITIALIZE ||
             method == MCP_METHOD_LOGGING_SET_LEVEL ||
             method == MCP_METHOD_PING || method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_SUBSCRIBE ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_RESOURCES_UNSUBSCRIBE ||
             method == MCP_METHOD_TASKS_CANCEL ||
             method == MCP_METHOD_TASKS_GET ||
             method == MCP_METHOD_TASKS_LIST ||
             method == MCP_METHOD_TASKS_RESULT ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
    case MCPProtocolVersion::V_2026_07_28:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_SERVER_DISCOVER ||
             method == MCP_METHOD_SUBSCRIPTIONS_LISTEN ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Whether a standalone server-to-client request is defined in this revision.
/// In 2026-07-28 these are nested MRTR requests, not JSON-RPC requests.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_server_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, "roots/list"));
/// assert(!sourcemeta::core::mcp_is_server_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "roots/list"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_is_server_request_method(const MCPProtocolVersion version,
                             const JSON::StringView method) noexcept -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
      return method == MCP_METHOD_PING || method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE;
    case MCPProtocolVersion::V_2025_06_18:
      return method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_PING || method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE;
    case MCPProtocolVersion::V_2025_11_25:
      return method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_PING || method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE ||
             method == MCP_METHOD_TASKS_CANCEL ||
             method == MCP_METHOD_TASKS_GET ||
             method == MCP_METHOD_TASKS_LIST ||
             method == MCP_METHOD_TASKS_RESULT;
    case MCPProtocolVersion::V_2026_07_28:
      return false;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Whether a notification method is defined in this revision.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "notifications/cancelled"));
/// assert(!sourcemeta::core::mcp_is_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "notifications/initialized"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_is_notification_method(const MCPProtocolVersion version,
                           const JSON::StringView method) noexcept -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
    case MCPProtocolVersion::V_2025_06_18:
      return method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
    case MCPProtocolVersion::V_2025_11_25:
      return method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_ELICITATION_COMPLETE ||
             method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_TASKS_STATUS ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
    case MCPProtocolVersion::V_2026_07_28:
      return method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_SUBSCRIPTIONS_ACKNOWLEDGED ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Whether a nested MRTR request method is defined in this revision.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_input_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "elicitation/create"));
/// assert(!sourcemeta::core::mcp_is_input_request_method(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///     "elicitation/create"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
constexpr auto
mcp_is_input_request_method(const MCPProtocolVersion version,
                            const JSON::StringView method) noexcept -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
    case MCPProtocolVersion::V_2025_06_18:
    case MCPProtocolVersion::V_2025_11_25:
      return false;
    case MCPProtocolVersion::V_2026_07_28:
      return method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Whether a method is defined by the base protocol, including nested requests.
/// This does not establish server implementation or negotiated extension
/// support.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_supports_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28, "server/discover"));
/// assert(!sourcemeta::core::mcp_supports_method(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25, "server/discover"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto mcp_supports_method(const MCPProtocolVersion version,
                                   const JSON::StringView method) noexcept
    -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_INITIALIZE ||
             method == MCP_METHOD_LOGGING_SET_LEVEL ||
             method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
             method == MCP_METHOD_PING || method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_SUBSCRIBE ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_RESOURCES_UNSUBSCRIBE ||
             method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
    case MCPProtocolVersion::V_2025_06_18:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_INITIALIZE ||
             method == MCP_METHOD_LOGGING_SET_LEVEL ||
             method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
             method == MCP_METHOD_PING || method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_SUBSCRIBE ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_RESOURCES_UNSUBSCRIBE ||
             method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
    case MCPProtocolVersion::V_2025_11_25:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_INITIALIZE ||
             method == MCP_METHOD_LOGGING_SET_LEVEL ||
             method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_ELICITATION_COMPLETE ||
             method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_TASKS_STATUS ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
             method == MCP_METHOD_PING || method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_SUBSCRIBE ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_RESOURCES_UNSUBSCRIBE ||
             method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE ||
             method == MCP_METHOD_TASKS_CANCEL ||
             method == MCP_METHOD_TASKS_GET ||
             method == MCP_METHOD_TASKS_LIST ||
             method == MCP_METHOD_TASKS_RESULT ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
    case MCPProtocolVersion::V_2026_07_28:
      return method == MCP_METHOD_COMPLETION_COMPLETE ||
             method == MCP_METHOD_ELICITATION_CREATE ||
             method == MCP_METHOD_NOTIFICATIONS_CANCELLED ||
             method == MCP_METHOD_NOTIFICATIONS_MESSAGE ||
             method == MCP_METHOD_NOTIFICATIONS_PROGRESS ||
             method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
             method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED ||
             method == MCP_METHOD_NOTIFICATIONS_SUBSCRIPTIONS_ACKNOWLEDGED ||
             method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
             method == MCP_METHOD_PROMPTS_GET ||
             method == MCP_METHOD_PROMPTS_LIST ||
             method == MCP_METHOD_RESOURCES_LIST ||
             method == MCP_METHOD_RESOURCES_READ ||
             method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
             method == MCP_METHOD_ROOTS_LIST ||
             method == MCP_METHOD_SAMPLING_CREATE_MESSAGE ||
             method == MCP_METHOD_SERVER_DISCOVER ||
             method == MCP_METHOD_SUBSCRIPTIONS_LISTEN ||
             method == MCP_METHOD_TOOLS_CALL || method == MCP_METHOD_TOOLS_LIST;
  }
  std::unreachable();
}

/// @ingroup mcp
/// Whether an operation may return an input_required result.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_mrtr_method("tools/call"));
/// assert(!sourcemeta::core::mcp_is_mrtr_method("tools/list"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr
constexpr auto mcp_is_mrtr_method(const JSON::StringView method) noexcept
    -> bool {
  return method == MCP_METHOD_TOOLS_CALL ||
         method == MCP_METHOD_RESOURCES_READ ||
         method == MCP_METHOD_PROMPTS_GET;
}

/// @ingroup mcp
/// Whether a complete operation result requires caching metadata in 2026-07-28.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_cacheable_method("tools/list"));
/// assert(!sourcemeta::core::mcp_is_cacheable_method("tools/call"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#cacheable-results
constexpr auto mcp_is_cacheable_method(const JSON::StringView method) noexcept
    -> bool {
  return method == MCP_METHOD_SERVER_DISCOVER ||
         method == MCP_METHOD_TOOLS_LIST || method == MCP_METHOD_PROMPTS_LIST ||
         method == MCP_METHOD_RESOURCES_LIST ||
         method == MCP_METHOD_RESOURCES_TEMPLATES_LIST ||
         method == MCP_METHOD_RESOURCES_READ;
}

/// @ingroup mcp
/// Check the base client notification direction for an exact revision.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_client_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2025_11_25,
///     "notifications/roots/list_changed"));
/// assert(!sourcemeta::core::mcp_is_client_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "notifications/roots/list_changed"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_is_client_notification_method(MCPProtocolVersion version,
                                  JSON::StringView method) noexcept -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
    case MCPProtocolVersion::V_2025_06_18:
      return method == "notifications/cancelled" ||
             method == "notifications/initialized" ||
             method == "notifications/progress" ||
             method == "notifications/roots/list_changed";
    case MCPProtocolVersion::V_2025_11_25:
      return method == "notifications/cancelled" ||
             method == "notifications/initialized" ||
             method == "notifications/progress" ||
             method == "notifications/tasks/status" ||
             method == "notifications/roots/list_changed";
    case MCPProtocolVersion::V_2026_07_28:
      return method == "notifications/cancelled";
  }
  std::unreachable();
}

/// @ingroup mcp
/// Check the base server notification direction for an exact revision.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/mcp.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::mcp_is_server_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "notifications/cancelled"));
/// assert(!sourcemeta::core::mcp_is_server_notification_method(
///     sourcemeta::core::MCPProtocolVersion::V_2026_07_28,
///     "notifications/initialized"));
/// ```
///
/// @see
/// https://modelcontextprotocol.io/specification/2026-07-28/schema
constexpr auto
mcp_is_server_notification_method(MCPProtocolVersion version,
                                  JSON::StringView method) noexcept -> bool {
  switch (version) {
    case MCPProtocolVersion::V_2025_03_26:
    case MCPProtocolVersion::V_2025_06_18:
      return method == "notifications/cancelled" ||
             method == "notifications/progress" ||
             method == "notifications/resources/list_changed" ||
             method == "notifications/resources/updated" ||
             method == "notifications/prompts/list_changed" ||
             method == "notifications/tools/list_changed" ||
             method == "notifications/message";
    case MCPProtocolVersion::V_2025_11_25:
      return method == "notifications/cancelled" ||
             method == "notifications/progress" ||
             method == "notifications/resources/list_changed" ||
             method == "notifications/resources/updated" ||
             method == "notifications/prompts/list_changed" ||
             method == "notifications/tools/list_changed" ||
             method == "notifications/tasks/status" ||
             method == "notifications/message" ||
             method == "notifications/elicitation/complete";
    case MCPProtocolVersion::V_2026_07_28:
      return method == "notifications/cancelled" ||
             method == "notifications/progress" ||
             method == "notifications/resources/list_changed" ||
             method == "notifications/subscriptions/acknowledged" ||
             method == "notifications/resources/updated" ||
             method == "notifications/prompts/list_changed" ||
             method == "notifications/tools/list_changed" ||
             method == "notifications/message";
  }
  std::unreachable();
}

} // namespace sourcemeta::core

#endif
