#include <sourcemeta/core/mcp.h>

#include "helpers.h"
#include "validation.h"

#include <algorithm> // std::find
#include <cmath>     // std::isfinite
#include <utility>   // std::move, std::pair

namespace {
using sourcemeta::core::JSON;

auto optional_string(const JSON &value, const JSON::StringView name,
                     const JSON::Object::hash_type hash) -> bool {
  const auto *field{value.try_at(name, hash)};
  return (field == nullptr) || field->is_string();
}

// The acknowledgment reflects only the subset the server agreed to honor, and
// a notification type the server does not support is omitted rather than
// acknowledged as false, so an event survives the intersection only when both
// sides opted into it
// https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions
auto opt_in(const std::optional<bool> requested,
            const std::optional<bool> supported) -> std::optional<bool> {
  return requested.value_or(false) && supported.value_or(false)
             ? std::optional<bool>{true}
             : std::nullopt;
}
} // namespace

namespace sourcemeta::core {
auto mcp_parse_subscription_filter(const MCPProtocolVersion version,
                                   const JSON &parameters)
    -> std::optional<MCPSubscriptionFilter> {
  if (!mcp_supports_subscriptions_listen(version) ||
      mcp_validate_request_parameters(parameters).first !=
          MCPRequestMetaStatus::Valid) {
    return std::nullopt;
  }
  const auto *filter{parameters.try_at(
      "notifications", sourcemeta::core::MCP_HASH_NOTIFICATIONS)};
  if ((filter == nullptr) || !filter->is_object()) {
    return std::nullopt;
  }
  MCPSubscriptionFilter result;
  for (const auto &[name, hash] :
       {std::pair{JSON::StringView{"toolsListChanged"},
                  sourcemeta::core::MCP_HASH_TOOLS_LIST_CHANGED},
        std::pair{JSON::StringView{"promptsListChanged"},
                  sourcemeta::core::MCP_HASH_PROMPTS_LIST_CHANGED},
        std::pair{JSON::StringView{"resourcesListChanged"},
                  sourcemeta::core::MCP_HASH_RESOURCES_LIST_CHANGED}}) {
    const auto *value{filter->try_at(name, hash)};
    if (value == nullptr) {
      continue;
    }
    if (!value->is_boolean()) {
      return std::nullopt;
    }
    if (JSON::StringView{name} == "toolsListChanged") {
      result.tools_list_changed = value->to_boolean();
    } else if (JSON::StringView{name} == "promptsListChanged") {
      result.prompts_list_changed = value->to_boolean();
    } else {
      result.resources_list_changed = value->to_boolean();
    }
  }
  if (const auto *uris{
          filter->try_at("resourceSubscriptions",
                         sourcemeta::core::MCP_HASH_RESOURCE_SUBSCRIPTIONS)};
      uris) {
    if (!uris->is_array()) {
      return std::nullopt;
    }
    result.resource_subscriptions.emplace();
    for (const auto &uri : uris->as_array()) {
      if (!uri.is_string()) {
        return std::nullopt;
      }
      result.resource_subscriptions->push_back(uri.to_string());
    }
  }
  return result;
}

auto mcp_intersect_subscription_filters(const MCPSubscriptionFilter &requested,
                                        const MCPSubscriptionFilter &supported)
    -> MCPSubscriptionFilter {
  MCPSubscriptionFilter result;
  result.tools_list_changed =
      opt_in(requested.tools_list_changed, supported.tools_list_changed);
  result.prompts_list_changed =
      opt_in(requested.prompts_list_changed, supported.prompts_list_changed);
  result.resources_list_changed = opt_in(requested.resources_list_changed,
                                         supported.resources_list_changed);
  if (requested.resource_subscriptions && supported.resource_subscriptions) {
    result.resource_subscriptions.emplace();
    for (const auto uri : *requested.resource_subscriptions) {
      if (std::find(supported.resource_subscriptions->begin(),
                    supported.resource_subscriptions->end(),
                    uri) != supported.resource_subscriptions->end()) {
        result.resource_subscriptions->push_back(uri);
      }
    }
  }
  return result;
}

auto mcp_result_type(const JSON &result) -> std::optional<JSON::StringView> {
  if (!result.is_object()) {
    return std::nullopt;
  }
  const auto *type{
      result.try_at("resultType", sourcemeta::core::MCP_HASH_RESULT_TYPE)};
  return (type == nullptr) ? std::optional<JSON::StringView>{"complete"}
         : type->is_string()
             ? std::optional<JSON::StringView>{type->to_string()}
             : std::nullopt;
}

auto mcp_resolve_result_type(
    const MCPProtocolVersion version, const JSON &result,
    const std::span<const JSON::StringView> supported_extension_types)
    -> std::optional<JSON::StringView> {
  const auto type{mcp_result_type(result)};
  if (!type) {
    return std::nullopt;
  }
  if (*type == "complete") {
    return type;
  }
  if (version != MCPProtocolVersion::V_2026_07_28) {
    return std::nullopt;
  }
  if (*type == "input_required") {
    return type;
  }
  return std::find(supported_extension_types.begin(),
                   supported_extension_types.end(),
                   *type) != supported_extension_types.end()
             ? type
             : std::nullopt;
}

auto mcp_make_notification(const MCPProtocolVersion version,
                           const bool from_server,
                           const JSON::StringView method, JSON parameters,
                           std::optional<JSON> subscription_id,
                           const MCPSubscriptionFilter *acknowledged,
                           const MCPRequestMeta *request_context) -> JSON {
  internal::require(from_server
                        ? mcp_is_server_notification_method(version, method)
                        : mcp_is_client_notification_method(version, method),
                    "Notification has an invalid revision or direction");
  internal::require(parameters.is_object() && internal::valid_meta(parameters),
                    "Invalid notification parameters");
  if (version == MCPProtocolVersion::V_2026_07_28) {
    if (const auto *meta{
            parameters.try_at("_meta", sourcemeta::core::MCP_HASH_META)};
        meta) {
      internal::require(internal::valid_metadata_object(*meta, false),
                        "Invalid notification metadata");
    }
  }
  if (version == MCPProtocolVersion::V_2026_07_28 && from_server) {
    if (method == MCP_METHOD_NOTIFICATIONS_MESSAGE) {
      internal::require((request_context != nullptr) &&
                            request_context->protocol_version == version &&
                            request_context->log_level,
                        "Logging requires an explicit per-request opt-in");
      internal::require(!subscription_id.has_value(),
                        "Logging cannot be sent on a subscription stream");
    } else if (method == MCP_METHOD_NOTIFICATIONS_PROGRESS) {
      const auto *token{parameters.try_at(
          "progressToken", sourcemeta::core::MCP_HASH_PROGRESS_TOKEN)};
      internal::require((request_context != nullptr) &&
                            request_context->protocol_version == version &&
                            (request_context->progress_token != nullptr) &&
                            (token != nullptr) &&
                            *token == *request_context->progress_token,
                        "Progress must echo the current request's token");
    }
  }
  if (subscription_id) {
    internal::require(version == MCPProtocolVersion::V_2026_07_28 &&
                          from_server && internal::valid_id(*subscription_id),
                      "Invalid subscription notification context");
  }
  if (method == MCP_METHOD_NOTIFICATIONS_PROGRESS) {
    const auto *token{parameters.try_at(
        "progressToken", sourcemeta::core::MCP_HASH_PROGRESS_TOKEN)};
    const auto *progress{
        parameters.try_at("progress", sourcemeta::core::MCP_HASH_PROGRESS)};
    const auto *total{
        parameters.try_at("total", sourcemeta::core::MCP_HASH_TOTAL)};
    internal::require(
        (token != nullptr) && internal::valid_id(*token) &&
            (progress != nullptr) && progress->is_number() &&
            std::isfinite(progress->as_real()) &&
            ((total == nullptr) ||
             (total->is_number() && std::isfinite(total->as_real()))) &&
            optional_string(parameters, "message",
                            sourcemeta::core::MCP_HASH_MESSAGE),
        "Invalid progress notification");
  } else if (method == MCP_METHOD_NOTIFICATIONS_MESSAGE) {
    const auto *level{
        parameters.try_at("level", sourcemeta::core::MCP_HASH_LEVEL)};
    internal::require(
        (level != nullptr) && level->is_string() &&
            internal::valid_log_level(level->to_string()) &&
            parameters.defines("data", sourcemeta::core::MCP_HASH_DATA) &&
            optional_string(parameters, "logger",
                            sourcemeta::core::MCP_HASH_LOGGER),
        "Invalid logging notification");
    if (version == MCPProtocolVersion::V_2026_07_28 && from_server) {
      const auto threshold{mcp_resolve_log_level(*request_context->log_level)};
      internal::require(
          threshold.has_value() &&
              mcp_log_level_enabled(*mcp_resolve_log_level(level->to_string()),
                                    *threshold),
          "Logging notification is below the request threshold");
    }
  } else if (method == MCP_METHOD_NOTIFICATIONS_CANCELLED) {
    const auto *identifier{
        parameters.try_at("requestId", sourcemeta::core::MCP_HASH_REQUEST_ID)};
    internal::require((identifier != nullptr) &&
                          internal::valid_id(*identifier) &&
                          optional_string(parameters, "reason",
                                          sourcemeta::core::MCP_HASH_REASON),
                      "Invalid cancellation notification");
    if (version == MCPProtocolVersion::V_2026_07_28 && from_server) {
      internal::require(
          subscription_id && *subscription_id == *identifier,
          "Servers may only cancel the originating listen request");
    }
  } else if (method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED) {
    internal::require(
        parameters.defines("uri", sourcemeta::core::MCP_HASH_URI) &&
            parameters.at("uri", sourcemeta::core::MCP_HASH_URI).is_string(),
        "Resource update requires a URI");
  } else if (method == MCP_METHOD_NOTIFICATIONS_ELICITATION_COMPLETE) {
    internal::require(
        parameters.defines("elicitationId",
                           sourcemeta::core::MCP_HASH_ELICITATION_ID) &&
            parameters
                .at("elicitationId", sourcemeta::core::MCP_HASH_ELICITATION_ID)
                .is_string(),
        "Elicitation completion requires an ID");
  } else {
    // Task status and acknowledgment have specialized payloads and must use
    // their owning extension or the acknowledgment builder.
    internal::require(
        method == MCP_METHOD_NOTIFICATIONS_INITIALIZED ||
            method == MCP_METHOD_NOTIFICATIONS_ROOTS_LIST_CHANGED ||
            method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
            method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
            method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED,
        "Use the specialized builder for this notification");
  }
  if (version == MCPProtocolVersion::V_2026_07_28 &&
      (method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED ||
       method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED ||
       method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED ||
       method == MCP_METHOD_NOTIFICATIONS_RESOURCES_UPDATED)) {
    internal::require(subscription_id && (acknowledged != nullptr),
                      "Updates require an acknowledged subscription");
    const bool enabled{
        method == MCP_METHOD_NOTIFICATIONS_TOOLS_LIST_CHANGED
            ? acknowledged->tools_list_changed.value_or(false)
        : method == MCP_METHOD_NOTIFICATIONS_PROMPTS_LIST_CHANGED
            ? acknowledged->prompts_list_changed.value_or(false)
        : method == MCP_METHOD_NOTIFICATIONS_RESOURCES_LIST_CHANGED
            ? acknowledged->resources_list_changed.value_or(false)
            : acknowledged->resource_subscriptions &&
                  std::find(acknowledged->resource_subscriptions->begin(),
                            acknowledged->resource_subscriptions->end(),
                            parameters.at("uri", sourcemeta::core::MCP_HASH_URI)
                                .to_string()) !=
                      acknowledged->resource_subscriptions->end()};
    internal::require(enabled, "Notification was not acknowledged");
  }
  if (subscription_id) {
    auto *meta{parameters.try_at("_meta", sourcemeta::core::MCP_HASH_META)};
    if (meta == nullptr) {
      parameters.assign_assume_new("_meta", JSON::make_object(),
                                   sourcemeta::core::MCP_HASH_META);
      meta = parameters.try_at("_meta", sourcemeta::core::MCP_HASH_META);
    }
    // Caller-supplied metadata may already name a subscription
    meta->assign("io.modelcontextprotocol/subscriptionId",
                 std::move(*subscription_id));
  }
  return jsonrpc_make_notification(method, std::move(parameters));
}
auto mcp_validate_continuation(const MCPProtocolVersion version,
                               const JSON &parameters,
                               const JSON &input_requests) -> bool {
  if (!mcp_supports_mrtr(version) || !input_requests.is_object() ||
      mcp_validate_request_parameters(parameters).first !=
          MCPRequestMetaStatus::Valid) {
    return false;
  }
  const auto *state{parameters.try_at(
      "requestState", sourcemeta::core::MCP_HASH_REQUEST_STATE)};
  const auto *responses{parameters.try_at(
      "inputResponses", sourcemeta::core::MCP_HASH_INPUT_RESPONSES)};
  if (((state == nullptr) && (responses == nullptr)) ||
      ((state != nullptr) && !state->is_string()) ||
      ((responses != nullptr) && !responses->is_object())) {
    return false;
  }
  if (responses == nullptr) {
    return true;
  }
  for (const auto &entry : responses->as_object()) {
    const auto *prior{input_requests.try_at(entry.first)};
    // MRTR permits unrecognized response information to be ignored.
    if (prior == nullptr) {
      continue;
    }
    if (!prior->is_object()) {
      return false;
    }
    const auto *method{
        prior->try_at("method", sourcemeta::core::MCP_HASH_METHOD)};
    if ((method == nullptr) || !method->is_string() ||
        !internal::valid_input_response(method->to_string(), *prior,
                                        entry.second)) {
      return false;
    }
  }
  return true;
}

auto mcp_validate_continuation(
    const MCPProtocolVersion version, const JSON &parameters,
    const JSON &input_requests,
    const std::optional<JSON::StringView> expected_state) -> bool {
  const auto *state{
      parameters.is_object()
          ? parameters.try_at("requestState", MCP_HASH_REQUEST_STATE)
          : nullptr};
  // A client that was given no state token must not invent one, so a server
  // that sent none can require its absence
  // https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr#client-requirements-basic-workflow
  if (!expected_state.has_value()) {
    return (state == nullptr) &&
           mcp_validate_continuation(version, parameters, input_requests);
  }
  return state != nullptr && state->is_string() &&
         state->to_string() == *expected_state &&
         mcp_validate_continuation(version, parameters, input_requests);
}

} // namespace sourcemeta::core
