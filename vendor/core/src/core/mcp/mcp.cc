#include <sourcemeta/core/mcp.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonrpc.h>

#include <algorithm> // std::clamp
#include <cassert>   // assert
#include <cmath>     // std::isnan
#include <cstddef>   // std::size_t
#include <optional>  // std::optional
#include <sstream>   // std::ostringstream
#include <string>    // std::string
#include <utility>   // std::move

namespace {

const auto MCP_HASH_ANNOTATIONS{
    sourcemeta::core::JSON::Object::hash("annotations")};
const auto MCP_HASH_ARGUMENTS{
    sourcemeta::core::JSON::Object::hash("arguments")};
const auto MCP_HASH_CAPABILITIES{
    sourcemeta::core::JSON::Object::hash("capabilities")};
const auto MCP_HASH_COMPLETIONS{
    sourcemeta::core::JSON::Object::hash("completions")};
const auto MCP_HASH_CONTENT{sourcemeta::core::JSON::Object::hash("content")};
const auto MCP_HASH_CONTENTS{sourcemeta::core::JSON::Object::hash("contents")};
const auto MCP_HASH_DESCRIPTION{
    sourcemeta::core::JSON::Object::hash("description")};
const auto MCP_HASH_DESTRUCTIVE_HINT{
    sourcemeta::core::JSON::Object::hash("destructiveHint")};
const auto MCP_HASH_IDEMPOTENT_HINT{
    sourcemeta::core::JSON::Object::hash("idempotentHint")};
const auto MCP_HASH_INPUT_SCHEMA{
    sourcemeta::core::JSON::Object::hash("inputSchema")};
const auto MCP_HASH_INSTRUCTIONS{
    sourcemeta::core::JSON::Object::hash("instructions")};
const auto MCP_HASH_IS_ERROR{sourcemeta::core::JSON::Object::hash("isError")};
const auto MCP_HASH_LOGGING{sourcemeta::core::JSON::Object::hash("logging")};
const auto MCP_HASH_MIME_TYPE{sourcemeta::core::JSON::Object::hash("mimeType")};
const auto MCP_HASH_NAME{sourcemeta::core::JSON::Object::hash("name")};
const auto MCP_HASH_OPEN_WORLD_HINT{
    sourcemeta::core::JSON::Object::hash("openWorldHint")};
const auto MCP_HASH_OUTPUT_SCHEMA{
    sourcemeta::core::JSON::Object::hash("outputSchema")};
const auto MCP_HASH_PRIORITY{sourcemeta::core::JSON::Object::hash("priority")};
const auto MCP_HASH_PROMPTS{sourcemeta::core::JSON::Object::hash("prompts")};
const auto MCP_HASH_PROTOCOL_VERSION{
    sourcemeta::core::JSON::Object::hash("protocolVersion")};
const auto MCP_HASH_READ_ONLY_HINT{
    sourcemeta::core::JSON::Object::hash("readOnlyHint")};
const auto MCP_HASH_RESOURCES{
    sourcemeta::core::JSON::Object::hash("resources")};
const auto MCP_HASH_SERVER_INFO{
    sourcemeta::core::JSON::Object::hash("serverInfo")};
const auto MCP_HASH_SIZE{sourcemeta::core::JSON::Object::hash("size")};
const auto MCP_HASH_STRUCTURED_CONTENT{
    sourcemeta::core::JSON::Object::hash("structuredContent")};
const auto MCP_HASH_TEXT{sourcemeta::core::JSON::Object::hash("text")};
const auto MCP_HASH_TITLE{sourcemeta::core::JSON::Object::hash("title")};
const auto MCP_HASH_TOOLS{sourcemeta::core::JSON::Object::hash("tools")};
const auto MCP_HASH_TYPE{sourcemeta::core::JSON::Object::hash("type")};
const auto MCP_HASH_URI{sourcemeta::core::JSON::Object::hash("uri")};
const auto MCP_HASH_URI_TEMPLATE{
    sourcemeta::core::JSON::Object::hash("uriTemplate")};
const auto MCP_HASH_VERSION{sourcemeta::core::JSON::Object::hash("version")};
const auto MCP_HASH_WEBSITE_URL{
    sourcemeta::core::JSON::Object::hash("websiteUrl")};

} // namespace

namespace sourcemeta::core {

auto mcp_make_text_block(const JSON::StringView text)
    -> sourcemeta::core::JSON {
  auto block{sourcemeta::core::JSON::make_object()};
  block.assign_assume_new("type", sourcemeta::core::JSON{"text"},
                          MCP_HASH_TYPE);
  block.assign_assume_new("text", sourcemeta::core::JSON{text}, MCP_HASH_TEXT);
  return block;
}

auto mcp_make_resource_link(const MCPProtocolVersion version,
                            const JSON::StringView uri,
                            const JSON::StringView mime_type,
                            const JSON::StringView name,
                            const JSON::StringView description)
    -> sourcemeta::core::JSON {
  if (!mcp_supports_resource_link_content(version)) {
    // Multi-line fallback is deliberate: the consumer is an LLM reading tool
    // output, not a human display surface, so a vertical layout keeps all three
    // fields readable while sidestepping quoting issues, etc. The URI is the
    // primary identifier and is always emitted, even when other fields are
    // empty
    std::string text;
    if (!name.empty()) {
      text.append(name);
      text.append("\n");
    }
    text.append(uri);
    if (!description.empty()) {
      text.append("\n");
      text.append(description);
    }
    return mcp_make_text_block(text);
  }

  auto block{sourcemeta::core::JSON::make_object()};
  block.assign_assume_new("type", sourcemeta::core::JSON{"resource_link"},
                          MCP_HASH_TYPE);
  block.assign_assume_new("uri", sourcemeta::core::JSON{uri}, MCP_HASH_URI);
  if (!name.empty()) {
    block.assign_assume_new("name", sourcemeta::core::JSON{name},
                            MCP_HASH_NAME);
  }
  if (!description.empty()) {
    block.assign_assume_new("description", sourcemeta::core::JSON{description},
                            MCP_HASH_DESCRIPTION);
  }
  block.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                          MCP_HASH_MIME_TYPE);
  return block;
}

auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON result)
    -> sourcemeta::core::JSON {
  std::ostringstream payload;
  sourcemeta::core::prettify(result, payload);

  auto content{sourcemeta::core::JSON::make_array()};
  content.push_back(mcp_make_text_block(payload.str()));

  auto envelope_result{sourcemeta::core::JSON::make_object()};
  envelope_result.assign_assume_new("content", std::move(content),
                                    MCP_HASH_CONTENT);
  if (mcp_supports_structured_content(version)) {
    envelope_result.assign_assume_new("structuredContent", std::move(result),
                                      MCP_HASH_STRUCTURED_CONTENT);
  }
  envelope_result.assign_assume_new("isError", sourcemeta::core::JSON{false},
                                    MCP_HASH_IS_ERROR);
  return sourcemeta::core::jsonrpc_make_success(identifier,
                                                std::move(envelope_result));
}

auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON structured,
                           sourcemeta::core::JSON content_blocks)
    -> sourcemeta::core::JSON {
  auto envelope_result{sourcemeta::core::JSON::make_object()};
  envelope_result.assign_assume_new("content", std::move(content_blocks),
                                    MCP_HASH_CONTENT);
  if (mcp_supports_structured_content(version)) {
    envelope_result.assign_assume_new("structuredContent",
                                      std::move(structured),
                                      MCP_HASH_STRUCTURED_CONTENT);
  }
  envelope_result.assign_assume_new("isError", sourcemeta::core::JSON{false},
                                    MCP_HASH_IS_ERROR);
  return sourcemeta::core::jsonrpc_make_success(identifier,
                                                std::move(envelope_result));
}

auto mcp_make_tool_error(const sourcemeta::core::JSON &identifier,
                         const JSON::StringView message)
    -> sourcemeta::core::JSON {
  auto content{sourcemeta::core::JSON::make_array()};
  content.push_back(mcp_make_text_block(message));

  auto envelope_result{sourcemeta::core::JSON::make_object()};
  envelope_result.assign_assume_new("content", std::move(content),
                                    MCP_HASH_CONTENT);
  envelope_result.assign_assume_new("isError", sourcemeta::core::JSON{true},
                                    MCP_HASH_IS_ERROR);
  return sourcemeta::core::jsonrpc_make_success(identifier,
                                                std::move(envelope_result));
}

auto mcp_make_error_resource_not_found(const sourcemeta::core::JSON &identifier)
    -> sourcemeta::core::JSON {
  return sourcemeta::core::jsonrpc_make_error(
      &identifier, MCP_CODE_RESOURCE_NOT_FOUND, "Resource not found");
}

auto mcp_make_resource(const JSON::StringView uri, const JSON::StringView name,
                       const JSON::StringView mime_type,
                       const JSON::StringView description,
                       const std::optional<std::size_t> size,
                       const std::optional<double> priority)
    -> sourcemeta::core::JSON {
  auto resource{sourcemeta::core::JSON::make_object()};
  resource.assign_assume_new("uri", sourcemeta::core::JSON{uri}, MCP_HASH_URI);
  resource.assign_assume_new("name", sourcemeta::core::JSON{name},
                             MCP_HASH_NAME);
  if (!description.empty()) {
    resource.assign_assume_new("description",
                               sourcemeta::core::JSON{description},
                               MCP_HASH_DESCRIPTION);
  }
  resource.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                             MCP_HASH_MIME_TYPE);
  if (size.has_value()) {
    resource.assign_assume_new("size", sourcemeta::core::JSON{size.value()},
                               MCP_HASH_SIZE);
  }
  if (priority.has_value()) {
    const auto priority_value{std::isnan(priority.value())
                                  ? 1.0
                                  : std::clamp(priority.value(), 0.0, 1.0)};
    auto annotations{sourcemeta::core::JSON::make_object()};
    annotations.assign_assume_new(
        "priority", sourcemeta::core::JSON{priority_value}, MCP_HASH_PRIORITY);
    resource.assign_assume_new("annotations", std::move(annotations),
                               MCP_HASH_ANNOTATIONS);
  }
  return resource;
}

auto mcp_make_resource_text_content(const JSON::StringView uri,
                                    const JSON::StringView mime_type,
                                    const JSON::StringView text)
    -> sourcemeta::core::JSON {
  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("uri", sourcemeta::core::JSON{uri}, MCP_HASH_URI);
  entry.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                          MCP_HASH_MIME_TYPE);
  entry.assign_assume_new("text", sourcemeta::core::JSON{text}, MCP_HASH_TEXT);
  return entry;
}

auto mcp_make_resources_read_result(sourcemeta::core::JSON contents)
    -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new("contents", std::move(contents), MCP_HASH_CONTENTS);
  return result;
}

auto mcp_make_resource_template(const JSON::StringView uri_template,
                                const JSON::StringView name,
                                const JSON::StringView description,
                                const JSON::StringView mime_type)
    -> sourcemeta::core::JSON {
  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("uriTemplate", sourcemeta::core::JSON{uri_template},
                          MCP_HASH_URI_TEMPLATE);
  entry.assign_assume_new("name", sourcemeta::core::JSON{name}, MCP_HASH_NAME);
  entry.assign_assume_new("description", sourcemeta::core::JSON{description},
                          MCP_HASH_DESCRIPTION);
  entry.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                          MCP_HASH_MIME_TYPE);
  return entry;
}

auto mcp_make_tool_descriptor(
    const MCPProtocolVersion version, const JSON::StringView name,
    const JSON::StringView description, sourcemeta::core::JSON input_schema,
    std::optional<sourcemeta::core::JSON> output_schema,
    const MCPToolAnnotations &annotations) -> sourcemeta::core::JSON {
  assert(!annotations.read_only || !annotations.destructive);
  assert(!annotations.read_only || annotations.idempotent);
  // The MCP spec requires `type: "object"` on tool input schemas.
  // https://modelcontextprotocol.io/specification/2025-06-18/server/tools
  assert(input_schema.is_object() &&
         input_schema.defines("type", MCP_HASH_TYPE) &&
         input_schema.at("type", MCP_HASH_TYPE).is_string() &&
         input_schema.at("type", MCP_HASH_TYPE).to_string() == "object");

  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("name", sourcemeta::core::JSON{name}, MCP_HASH_NAME);
  entry.assign_assume_new("description", sourcemeta::core::JSON{description},
                          MCP_HASH_DESCRIPTION);
  entry.assign_assume_new("inputSchema", std::move(input_schema),
                          MCP_HASH_INPUT_SCHEMA);
  if (output_schema.has_value() && mcp_supports_output_schema(version)) {
    entry.assign_assume_new("outputSchema", std::move(output_schema).value(),
                            MCP_HASH_OUTPUT_SCHEMA);
  }

  auto annotations_object{sourcemeta::core::JSON::make_object()};
  if (!annotations.title.empty()) {
    annotations_object.assign_assume_new(
        "title", sourcemeta::core::JSON{annotations.title}, MCP_HASH_TITLE);
  }
  annotations_object.assign_assume_new(
      "readOnlyHint", sourcemeta::core::JSON{annotations.read_only},
      MCP_HASH_READ_ONLY_HINT);
  annotations_object.assign_assume_new(
      "destructiveHint", sourcemeta::core::JSON{annotations.destructive},
      MCP_HASH_DESTRUCTIVE_HINT);
  annotations_object.assign_assume_new(
      "idempotentHint", sourcemeta::core::JSON{annotations.idempotent},
      MCP_HASH_IDEMPOTENT_HINT);
  annotations_object.assign_assume_new(
      "openWorldHint", sourcemeta::core::JSON{annotations.open_world},
      MCP_HASH_OPEN_WORLD_HINT);
  entry.assign_assume_new("annotations", std::move(annotations_object),
                          MCP_HASH_ANNOTATIONS);

  return entry;
}

auto mcp_make_initialize_result(const sourcemeta::core::JSON &request,
                                const MCPServerCapabilities &capabilities,
                                const MCPImplementation &server,
                                const JSON::StringView instructions)
    -> sourcemeta::core::JSON {
  const auto *identifier{sourcemeta::core::jsonrpc_request_id(request)};
  const auto *parameters{sourcemeta::core::jsonrpc_params(request)};
  if (identifier == nullptr || parameters == nullptr ||
      !parameters->is_object()) {
    return sourcemeta::core::jsonrpc_make_error_invalid_request(identifier);
  }

  JSON::StringView requested_version{};
  const auto *protocol_version_field{
      parameters->try_at("protocolVersion", MCP_HASH_PROTOCOL_VERSION)};
  if (protocol_version_field != nullptr &&
      protocol_version_field->is_string()) {
    requested_version = protocol_version_field->to_string();
  }
  const auto resolved{mcp_resolve_protocol_version(requested_version)};
  // MCP lifecycle, version negotiation: "If the server supports the requested
  // protocol version, it MUST respond with the same version. Otherwise, the
  // server MUST respond with another protocol version it supports. This SHOULD
  // be the latest version supported by the server."
  // https://modelcontextprotocol.io/specification/2025-06-18/basic/lifecycle#version-negotiation
  const auto version{resolved.value_or(MCPProtocolVersion::V_2025_11_25)};

  auto capabilities_object{sourcemeta::core::JSON::make_object()};
  if (capabilities.prompts) {
    capabilities_object.assign_assume_new(
        "prompts", sourcemeta::core::JSON::make_object(), MCP_HASH_PROMPTS);
  }
  if (capabilities.resources) {
    capabilities_object.assign_assume_new(
        "resources", sourcemeta::core::JSON::make_object(), MCP_HASH_RESOURCES);
  }
  if (capabilities.tools) {
    capabilities_object.assign_assume_new(
        "tools", sourcemeta::core::JSON::make_object(), MCP_HASH_TOOLS);
  }
  if (capabilities.logging) {
    capabilities_object.assign_assume_new(
        "logging", sourcemeta::core::JSON::make_object(), MCP_HASH_LOGGING);
  }
  if (capabilities.completions) {
    capabilities_object.assign_assume_new("completions",
                                          sourcemeta::core::JSON::make_object(),
                                          MCP_HASH_COMPLETIONS);
  }

  auto server_info{sourcemeta::core::JSON::make_object()};
  server_info.assign_assume_new("name", sourcemeta::core::JSON{server.name},
                                MCP_HASH_NAME);
  server_info.assign_assume_new(
      "version", sourcemeta::core::JSON{server.version}, MCP_HASH_VERSION);
  if (!server.title.empty() && mcp_supports_implementation_title(version)) {
    server_info.assign_assume_new("title", sourcemeta::core::JSON{server.title},
                                  MCP_HASH_TITLE);
  }
  if (!server.description.empty() &&
      mcp_supports_implementation_description(version)) {
    server_info.assign_assume_new("description",
                                  sourcemeta::core::JSON{server.description},
                                  MCP_HASH_DESCRIPTION);
  }
  if (!server.website_url.empty() &&
      mcp_supports_implementation_website_url(version)) {
    server_info.assign_assume_new("websiteUrl",
                                  sourcemeta::core::JSON{server.website_url},
                                  MCP_HASH_WEBSITE_URL);
  }

  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new(
      "protocolVersion",
      sourcemeta::core::JSON{mcp_protocol_version_string(version)},
      MCP_HASH_PROTOCOL_VERSION);
  result.assign_assume_new("capabilities", std::move(capabilities_object),
                           MCP_HASH_CAPABILITIES);
  result.assign_assume_new("serverInfo", std::move(server_info),
                           MCP_HASH_SERVER_INFO);
  if (!instructions.empty()) {
    result.assign_assume_new("instructions",
                             sourcemeta::core::JSON{instructions},
                             MCP_HASH_INSTRUCTIONS);
  }
  return sourcemeta::core::jsonrpc_make_success(*identifier, std::move(result));
}

auto mcp_tool_call_arguments(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  const auto *parameters{sourcemeta::core::jsonrpc_params(envelope)};
  if (parameters == nullptr || !parameters->is_object()) {
    return nullptr;
  }
  const auto *arguments{parameters->try_at("arguments", MCP_HASH_ARGUMENTS)};
  if (arguments == nullptr || !arguments->is_object()) {
    return nullptr;
  }
  return arguments;
}

} // namespace sourcemeta::core
