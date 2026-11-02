#ifndef SOURCEMETA_CORE_MCP_HELPERS_H_
#define SOURCEMETA_CORE_MCP_HELPERS_H_

#include <sourcemeta/core/json.h>

#include <algorithm>   // std::all_of
#include <string_view> // std::string_view, std::string_view_literals

namespace sourcemeta::core {

using namespace std::string_view_literals;
inline constexpr auto MCP_HASH_ANNOTATIONS{
    sourcemeta::core::JSON::Object::hash("annotations"sv)};
inline constexpr auto MCP_HASH_ARGUMENTS{
    sourcemeta::core::JSON::Object::hash("arguments"sv)};
inline constexpr auto MCP_HASH_BODY_VALUE{
    sourcemeta::core::JSON::Object::hash("bodyValue"sv)};
inline constexpr auto MCP_HASH_CACHE_SCOPE{
    sourcemeta::core::JSON::Object::hash("cacheScope"sv)};
inline constexpr auto MCP_HASH_CAPABILITIES{
    sourcemeta::core::JSON::Object::hash("capabilities"sv)};
inline constexpr auto MCP_HASH_COMPLETIONS{
    sourcemeta::core::JSON::Object::hash("completions"sv)};
inline constexpr auto MCP_HASH_CONTENT{
    sourcemeta::core::JSON::Object::hash("content"sv)};
inline constexpr auto MCP_HASH_CONTENTS{
    sourcemeta::core::JSON::Object::hash("contents"sv)};
inline constexpr auto MCP_HASH_CONTEXT{
    sourcemeta::core::JSON::Object::hash("context"sv)};
inline constexpr auto MCP_HASH_DESCRIPTION{
    sourcemeta::core::JSON::Object::hash("description"sv)};
inline constexpr auto MCP_HASH_DESTRUCTIVE_HINT{
    sourcemeta::core::JSON::Object::hash("destructiveHint"sv)};
inline constexpr auto MCP_HASH_ELICITATION{
    sourcemeta::core::JSON::Object::hash("elicitation"sv)};
inline constexpr auto MCP_HASH_EXPERIMENTAL{
    sourcemeta::core::JSON::Object::hash("experimental"sv)};
inline constexpr auto MCP_HASH_EXTENSIONS{
    sourcemeta::core::JSON::Object::hash("extensions"sv)};
inline constexpr auto MCP_HASH_FORM{
    sourcemeta::core::JSON::Object::hash("form"sv)};
inline constexpr auto MCP_HASH_HEADER{
    sourcemeta::core::JSON::Object::hash("header"sv)};
inline constexpr auto MCP_HASH_HEADER_VALUE{
    sourcemeta::core::JSON::Object::hash("headerValue"sv)};
inline constexpr auto MCP_HASH_ID{sourcemeta::core::JSON::Object::hash("id"sv)};
inline constexpr auto MCP_HASH_IDEMPOTENT_HINT{
    sourcemeta::core::JSON::Object::hash("idempotentHint"sv)};
inline constexpr auto MCP_HASH_INPUT_REQUESTS{
    sourcemeta::core::JSON::Object::hash("inputRequests"sv)};
inline constexpr auto MCP_HASH_INPUT_RESPONSES{
    sourcemeta::core::JSON::Object::hash("inputResponses"sv)};
inline constexpr auto MCP_HASH_INPUT_SCHEMA{
    sourcemeta::core::JSON::Object::hash("inputSchema"sv)};
inline constexpr auto MCP_HASH_INSTRUCTIONS{
    sourcemeta::core::JSON::Object::hash("instructions"sv)};
inline constexpr auto MCP_HASH_IS_ERROR{
    sourcemeta::core::JSON::Object::hash("isError"sv)};
inline constexpr auto MCP_HASH_JSONRPC{
    sourcemeta::core::JSON::Object::hash("jsonrpc"sv)};
inline constexpr auto MCP_HASH_LIST_CHANGED{
    sourcemeta::core::JSON::Object::hash("listChanged"sv)};
inline constexpr auto MCP_HASH_LOGGING{
    sourcemeta::core::JSON::Object::hash("logging"sv)};
inline constexpr auto MCP_HASH_META{
    sourcemeta::core::JSON::Object::hash("_meta"sv)};
inline constexpr auto MCP_HASH_META_CLIENT_CAPABILITIES{
    sourcemeta::core::JSON::Object::hash(
        "io.modelcontextprotocol/clientCapabilities"sv)};
inline constexpr auto MCP_HASH_META_CLIENT_INFO{
    sourcemeta::core::JSON::Object::hash(
        "io.modelcontextprotocol/clientInfo"sv)};
inline constexpr auto MCP_HASH_META_LOG_LEVEL{
    sourcemeta::core::JSON::Object::hash("io.modelcontextprotocol/logLevel"sv)};
inline constexpr auto MCP_HASH_META_PROTOCOL_VERSION{
    sourcemeta::core::JSON::Object::hash(
        "io.modelcontextprotocol/protocolVersion"sv)};
inline constexpr auto MCP_HASH_META_SERVER_INFO{
    sourcemeta::core::JSON::Object::hash(
        "io.modelcontextprotocol/serverInfo"sv)};
inline constexpr auto MCP_HASH_META_SUBSCRIPTION_ID{
    sourcemeta::core::JSON::Object::hash(
        "io.modelcontextprotocol/subscriptionId"sv)};
inline constexpr auto MCP_HASH_METHOD{
    sourcemeta::core::JSON::Object::hash("method"sv)};
inline constexpr auto MCP_HASH_MIME_TYPE{
    sourcemeta::core::JSON::Object::hash("mimeType"sv)};
inline constexpr auto MCP_HASH_NAME{
    sourcemeta::core::JSON::Object::hash("name"sv)};
inline constexpr auto MCP_HASH_NEXT_CURSOR{
    sourcemeta::core::JSON::Object::hash("nextCursor"sv)};
inline constexpr auto MCP_HASH_NOTIFICATIONS{
    sourcemeta::core::JSON::Object::hash("notifications"sv)};
inline constexpr auto MCP_HASH_OPEN_WORLD_HINT{
    sourcemeta::core::JSON::Object::hash("openWorldHint"sv)};
inline constexpr auto MCP_HASH_OUTPUT_SCHEMA{
    sourcemeta::core::JSON::Object::hash("outputSchema"sv)};
inline constexpr auto MCP_HASH_PARAMS{
    sourcemeta::core::JSON::Object::hash("params"sv)};
inline constexpr auto MCP_HASH_PRIORITY{
    sourcemeta::core::JSON::Object::hash("priority"sv)};
inline constexpr auto MCP_HASH_PROMPTS{
    sourcemeta::core::JSON::Object::hash("prompts"sv)};
inline constexpr auto MCP_HASH_PROTOCOL_VERSION{
    sourcemeta::core::JSON::Object::hash("protocolVersion"sv)};
inline constexpr auto MCP_HASH_READ_ONLY_HINT{
    sourcemeta::core::JSON::Object::hash("readOnlyHint"sv)};
inline constexpr auto MCP_HASH_REQUEST_STATE{
    sourcemeta::core::JSON::Object::hash("requestState"sv)};
inline constexpr auto MCP_HASH_REQUESTED{
    sourcemeta::core::JSON::Object::hash("requested"sv)};
inline constexpr auto MCP_HASH_REQUIRED_CAPABILITIES{
    sourcemeta::core::JSON::Object::hash("requiredCapabilities"sv)};
inline constexpr auto MCP_HASH_RESOURCE_TEMPLATES{
    sourcemeta::core::JSON::Object::hash("resourceTemplates"sv)};
inline constexpr auto MCP_HASH_RESOURCES{
    sourcemeta::core::JSON::Object::hash("resources"sv)};
inline constexpr auto MCP_HASH_RESULT{
    sourcemeta::core::JSON::Object::hash("result"sv)};
inline constexpr auto MCP_HASH_RESULT_TYPE{
    sourcemeta::core::JSON::Object::hash("resultType"sv)};
inline constexpr auto MCP_HASH_ROOTS{
    sourcemeta::core::JSON::Object::hash("roots"sv)};
inline constexpr auto MCP_HASH_SAMPLING{
    sourcemeta::core::JSON::Object::hash("sampling"sv)};
inline constexpr auto MCP_HASH_SERVER_INFO{
    sourcemeta::core::JSON::Object::hash("serverInfo"sv)};
inline constexpr auto MCP_HASH_SIZE{
    sourcemeta::core::JSON::Object::hash("size"sv)};
inline constexpr auto MCP_HASH_STRUCTURED_CONTENT{
    sourcemeta::core::JSON::Object::hash("structuredContent"sv)};
inline constexpr auto MCP_HASH_SUBSCRIBE{
    sourcemeta::core::JSON::Object::hash("subscribe"sv)};
inline constexpr auto MCP_HASH_SUPPORTED{
    sourcemeta::core::JSON::Object::hash("supported"sv)};
inline constexpr auto MCP_HASH_SUPPORTED_VERSIONS{
    sourcemeta::core::JSON::Object::hash("supportedVersions"sv)};
inline constexpr auto MCP_HASH_TEXT{
    sourcemeta::core::JSON::Object::hash("text"sv)};
inline constexpr auto MCP_HASH_TITLE{
    sourcemeta::core::JSON::Object::hash("title"sv)};
inline constexpr auto MCP_HASH_TOOLS{
    sourcemeta::core::JSON::Object::hash("tools"sv)};
inline constexpr auto MCP_HASH_TTL_MS{
    sourcemeta::core::JSON::Object::hash("ttlMs"sv)};
inline constexpr auto MCP_HASH_TYPE{
    sourcemeta::core::JSON::Object::hash("type"sv)};
inline constexpr auto MCP_HASH_URI{
    sourcemeta::core::JSON::Object::hash("uri"sv)};
inline constexpr auto MCP_HASH_URI_TEMPLATE{
    sourcemeta::core::JSON::Object::hash("uriTemplate"sv)};
inline constexpr auto MCP_HASH_URL{
    sourcemeta::core::JSON::Object::hash("url"sv)};
inline constexpr auto MCP_HASH_VERSION{
    sourcemeta::core::JSON::Object::hash("version"sv)};
inline constexpr auto MCP_HASH_WEBSITE_URL{
    sourcemeta::core::JSON::Object::hash("websiteUrl"sv)};
inline constexpr auto MCP_HASH_ACTION{
    sourcemeta::core::JSON::Object::hash("action"sv)};
inline constexpr auto MCP_HASH_AUDIENCE{
    sourcemeta::core::JSON::Object::hash("audience"sv)};
inline constexpr auto MCP_HASH_BAGGAGE{
    sourcemeta::core::JSON::Object::hash("baggage"sv)};
inline constexpr auto MCP_HASH_BLOB{
    sourcemeta::core::JSON::Object::hash("blob"sv)};
inline constexpr auto MCP_HASH_CALL{
    sourcemeta::core::JSON::Object::hash("call"sv)};
inline constexpr auto MCP_HASH_CANCEL{
    sourcemeta::core::JSON::Object::hash("cancel"sv)};
inline constexpr auto MCP_HASH_CLIENT_INFO{
    sourcemeta::core::JSON::Object::hash("clientInfo"sv)};
inline constexpr auto MCP_HASH_COMPLETION{
    sourcemeta::core::JSON::Object::hash("completion"sv)};
inline constexpr auto MCP_HASH_COST_PRIORITY{
    sourcemeta::core::JSON::Object::hash("costPriority"sv)};
inline constexpr auto MCP_HASH_CREATE{
    sourcemeta::core::JSON::Object::hash("create"sv)};
inline constexpr auto MCP_HASH_CREATE_MESSAGE{
    sourcemeta::core::JSON::Object::hash("createMessage"sv)};
inline constexpr auto MCP_HASH_DATA{
    sourcemeta::core::JSON::Object::hash("data"sv)};
inline constexpr auto MCP_HASH_ELICITATION_ID{
    sourcemeta::core::JSON::Object::hash("elicitationId"sv)};
inline constexpr auto MCP_HASH_ERROR{
    sourcemeta::core::JSON::Object::hash("error"sv)};
inline constexpr auto MCP_HASH_HAS_MORE{
    sourcemeta::core::JSON::Object::hash("hasMore"sv)};
inline constexpr auto MCP_HASH_HINTS{
    sourcemeta::core::JSON::Object::hash("hints"sv)};
inline constexpr auto MCP_HASH_ICONS{
    sourcemeta::core::JSON::Object::hash("icons"sv)};
inline constexpr auto MCP_HASH_INCLUDE_CONTEXT{
    sourcemeta::core::JSON::Object::hash("includeContext"sv)};
inline constexpr auto MCP_HASH_INPUT{
    sourcemeta::core::JSON::Object::hash("input"sv)};
inline constexpr auto MCP_HASH_INTELLIGENCE_PRIORITY{
    sourcemeta::core::JSON::Object::hash("intelligencePriority"sv)};
inline constexpr auto MCP_HASH_LAST_MODIFIED{
    sourcemeta::core::JSON::Object::hash("lastModified"sv)};
inline constexpr auto MCP_HASH_LEVEL{
    sourcemeta::core::JSON::Object::hash("level"sv)};
inline constexpr auto MCP_HASH_LIST{
    sourcemeta::core::JSON::Object::hash("list"sv)};
inline constexpr auto MCP_HASH_LOGGER{
    sourcemeta::core::JSON::Object::hash("logger"sv)};
inline constexpr auto MCP_HASH_MAX_TOKENS{
    sourcemeta::core::JSON::Object::hash("maxTokens"sv)};
inline constexpr auto MCP_HASH_MESSAGE{
    sourcemeta::core::JSON::Object::hash("message"sv)};
inline constexpr auto MCP_HASH_MESSAGES{
    sourcemeta::core::JSON::Object::hash("messages"sv)};
inline constexpr auto MCP_HASH_METADATA{
    sourcemeta::core::JSON::Object::hash("metadata"sv)};
inline constexpr auto MCP_HASH_MODE{
    sourcemeta::core::JSON::Object::hash("mode"sv)};
inline constexpr auto MCP_HASH_MODEL{
    sourcemeta::core::JSON::Object::hash("model"sv)};
inline constexpr auto MCP_HASH_MODEL_PREFERENCES{
    sourcemeta::core::JSON::Object::hash("modelPreferences"sv)};
inline constexpr auto MCP_HASH_PROGRESS{
    sourcemeta::core::JSON::Object::hash("progress"sv)};
inline constexpr auto MCP_HASH_PROGRESS_TOKEN{
    sourcemeta::core::JSON::Object::hash("progressToken"sv)};
inline constexpr auto MCP_HASH_PROMPTS_LIST_CHANGED{
    sourcemeta::core::JSON::Object::hash("promptsListChanged"sv)};
inline constexpr auto MCP_HASH_PROPERTIES{
    sourcemeta::core::JSON::Object::hash("properties"sv)};
inline constexpr auto MCP_HASH_REASON{
    sourcemeta::core::JSON::Object::hash("reason"sv)};
inline constexpr auto MCP_HASH_REQUEST_ID{
    sourcemeta::core::JSON::Object::hash("requestId"sv)};
inline constexpr auto MCP_HASH_REQUESTED_SCHEMA{
    sourcemeta::core::JSON::Object::hash("requestedSchema"sv)};
inline constexpr auto MCP_HASH_REQUESTS{
    sourcemeta::core::JSON::Object::hash("requests"sv)};
inline constexpr auto MCP_HASH_REQUIRED{
    sourcemeta::core::JSON::Object::hash("required"sv)};
inline constexpr auto MCP_HASH_RESOURCE{
    sourcemeta::core::JSON::Object::hash("resource"sv)};
inline constexpr auto MCP_HASH_RESOURCE_SUBSCRIPTIONS{
    sourcemeta::core::JSON::Object::hash("resourceSubscriptions"sv)};
inline constexpr auto MCP_HASH_RESOURCES_LIST_CHANGED{
    sourcemeta::core::JSON::Object::hash("resourcesListChanged"sv)};
inline constexpr auto MCP_HASH_ROLE{
    sourcemeta::core::JSON::Object::hash("role"sv)};
inline constexpr auto MCP_HASH_SIZES{
    sourcemeta::core::JSON::Object::hash("sizes"sv)};
inline constexpr auto MCP_HASH_SPEED_PRIORITY{
    sourcemeta::core::JSON::Object::hash("speedPriority"sv)};
inline constexpr auto MCP_HASH_SRC{
    sourcemeta::core::JSON::Object::hash("src"sv)};
inline constexpr auto MCP_HASH_STOP_REASON{
    sourcemeta::core::JSON::Object::hash("stopReason"sv)};
inline constexpr auto MCP_HASH_STOP_SEQUENCES{
    sourcemeta::core::JSON::Object::hash("stopSequences"sv)};
inline constexpr auto MCP_HASH_SYSTEM_PROMPT{
    sourcemeta::core::JSON::Object::hash("systemPrompt"sv)};
inline constexpr auto MCP_HASH_TASKS{
    sourcemeta::core::JSON::Object::hash("tasks"sv)};
inline constexpr auto MCP_HASH_TEMPERATURE{
    sourcemeta::core::JSON::Object::hash("temperature"sv)};
inline constexpr auto MCP_HASH_THEME{
    sourcemeta::core::JSON::Object::hash("theme"sv)};
inline constexpr auto MCP_HASH_TOOL_CHOICE{
    sourcemeta::core::JSON::Object::hash("toolChoice"sv)};
inline constexpr auto MCP_HASH_TOOL_USE_ID{
    sourcemeta::core::JSON::Object::hash("toolUseId"sv)};
inline constexpr auto MCP_HASH_TOOLS_LIST_CHANGED{
    sourcemeta::core::JSON::Object::hash("toolsListChanged"sv)};
inline constexpr auto MCP_HASH_TOTAL{
    sourcemeta::core::JSON::Object::hash("total"sv)};
inline constexpr auto MCP_HASH_TRACEPARENT{
    sourcemeta::core::JSON::Object::hash("traceparent"sv)};
inline constexpr auto MCP_HASH_TRACESTATE{
    sourcemeta::core::JSON::Object::hash("tracestate"sv)};
inline constexpr auto MCP_HASH_VALUES{
    sourcemeta::core::JSON::Object::hash("values"sv)};

// A value travels literally when it can be carried in a field without help.
// The specification names when it cannot, and so must be encoded instead:
// "it contains non-ASCII characters, control characters, or has
// leading/trailing whitespace".
//
// This is narrower than what a field value may hold, and deliberately so. The
// same passage says such a value may carry "visible ASCII characters
// (0x21-0x7E), space (0x20), and horizontal tab (0x09)", yet a horizontal tab
// is itself a control character, which the sentence above sends to the encoded
// form. The two readings only disagree about the tab, and encoding it is the
// safe side of that disagreement, since an encoded value is always
// understood while a tab in a field value is the kind of thing an
// intermediary may fold or strip.
//
// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#value-encoding
inline auto mcp_is_plain_header_value(const JSON::StringView value) noexcept
    -> bool {
  if (!value.empty() && (value.front() == ' ' || value.back() == ' ')) {
    return false;
  }

  return std::all_of(value.begin(), value.end(), [](const char character) {
    const auto byte{static_cast<unsigned char>(character)};
    return byte >= 0x20 && byte < 0x7f;
  });
}

// The markers "are case-sensitive and MUST appear exactly as shown
// (lowercase)", so these are compared rather than matched loosely
// https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#value-encoding
inline constexpr auto MCP_HEADER_SENTINEL_PREFIX{"=?base64?"sv};
inline constexpr auto MCP_HEADER_SENTINEL_SUFFIX{"?="sv};

// The shortest form accepted is the two markers alone, around an empty body.
// A conforming producer never emits one, since a value that encodes to nothing
// is empty and an empty value needs no encoding, but reading one back costs
// nothing and the original rule was no stricter
inline auto mcp_is_encoded_header_value(const JSON::StringView value) noexcept
    -> bool {
  return value.starts_with(MCP_HEADER_SENTINEL_PREFIX) &&
         value.ends_with(MCP_HEADER_SENTINEL_SUFFIX) &&
         value.size() >= MCP_HEADER_SENTINEL_PREFIX.size() +
                             MCP_HEADER_SENTINEL_SUFFIX.size();
}

// What the two markers enclose, which is the base64 the encoded form carries
inline auto mcp_header_sentinel_body(const JSON::StringView value) noexcept
    -> JSON::StringView {
  return value.substr(MCP_HEADER_SENTINEL_PREFIX.size(),
                      value.size() - MCP_HEADER_SENTINEL_PREFIX.size() -
                          MCP_HEADER_SENTINEL_SUFFIX.size());
}

} // namespace sourcemeta::core

#endif
