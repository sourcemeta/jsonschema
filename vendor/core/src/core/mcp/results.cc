#include <sourcemeta/core/mcp.h>

#include "helpers.h"
#include "validation.h"

#include <sourcemeta/core/http.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonrpc.h>
#include <sourcemeta/core/uri.h>

#include <algorithm> // std::find
#include <cmath>     // std::isfinite
#include <optional>  // std::optional, std::nullopt
#include <span>      // std::span
#include <sstream>   // std::ostringstream
#include <utility>   // std::cmp_greater_equal, std::move

namespace {

auto checked_success(const sourcemeta::core::JSON &identifier,
                     sourcemeta::core::JSON result) -> sourcemeta::core::JSON {
  sourcemeta::core::internal::require(
      sourcemeta::core::internal::valid_id(identifier),
      "MCP identifiers must be strings or integers");
  return sourcemeta::core::jsonrpc_make_success(identifier, std::move(result));
}

auto require_array(const sourcemeta::core::JSON &value) -> void {
  sourcemeta::core::internal::require(value.is_array(),
                                      "MCP result entries must be an array");
}

auto apply_presentation(
    const sourcemeta::core::MCPProtocolVersion version,
    sourcemeta::core::JSON &value,
    const sourcemeta::core::MCPDescriptorPresentation &presentation) -> void {
  using namespace sourcemeta::core;
  if (mcp_protocol_version_at_least(version,
                                    MCPProtocolVersion::V_2025_06_18) &&
      !presentation.title.empty()) {
    value.assign_assume_new(JSON::String{"title"}, JSON{presentation.title},
                            MCP_HASH_TITLE);
  }
  if (mcp_protocol_version_at_least(version,
                                    MCPProtocolVersion::V_2025_11_25) &&
      presentation.icons != nullptr) {
    value.assign_assume_new(JSON::String{"icons"}, JSON{*presentation.icons},
                            MCP_HASH_ICONS);
    internal::require(internal::validation::icons(value),
                      "Invalid descriptor icons");
  }
}

} // namespace

namespace sourcemeta::core {

auto mcp_serialize_resource_annotations(
    const MCPProtocolVersion version, const MCPResourceAnnotations &annotations)
    -> JSON {
  auto result{JSON::make_object()};
  if (annotations.audience.has_value()) {
    auto audience{JSON::make_array()};
    for (const auto role : *annotations.audience) {
      internal::require(role == "user" || role == "assistant",
                        "Invalid annotation audience");
      audience.push_back(JSON{role});
    }
    result.assign_assume_new("audience", std::move(audience),
                             MCP_HASH_AUDIENCE);
  }
  if (annotations.priority.has_value()) {
    // The schema bounds this between zero and one, and JSON has no
    // representation for infinity or NaN, so neither a non-finite input nor
    // one outside the bounds is a priority this builder can express. Refusing
    // rather than folding the value in is what keeps a caller of the typed
    // struct and a caller handing over a descriptor the same answer
    // https://modelcontextprotocol.io/specification/2026-07-28/schema#annotations
    internal::require(std::isfinite(*annotations.priority),
                      "Annotation priority must be a finite number");
    internal::require(*annotations.priority >= 0.0 &&
                          *annotations.priority <= 1.0,
                      "Annotation priority must be between zero and one");
    result.assign_assume_new("priority", JSON{*annotations.priority},
                             MCP_HASH_PRIORITY);
  }
  if (mcp_protocol_version_at_least(version,
                                    MCPProtocolVersion::V_2025_06_18) &&
      !annotations.last_modified.empty()) {
    result.assign_assume_new("lastModified", JSON{annotations.last_modified},
                             MCP_HASH_LAST_MODIFIED);
  }
  return result;
}

namespace {
// A resource built through one of these carries the same two fields a
// descriptor passing through a list builder is held to, so building one
// directly is held to them as well rather than slipping past
auto require_resource_contents(const JSON::StringView uri,
                               const JSON::StringView mime_type) -> void {
  internal::require(
      sourcemeta::core::URI::is_uri(uri) &&
          (mime_type.empty() ||
           sourcemeta::core::http_parse_media_type(mime_type).has_value()),
      "Invalid resource contents");
}

auto media_block(const MCPProtocolVersion version, const JSON::StringView type,
                 const JSON::StringView data, const JSON::StringView mime_type,
                 const MCPResourceAnnotations &annotations) -> JSON {
  auto result{JSON::make_object()};
  result.assign_assume_new("type", JSON{type}, MCP_HASH_TYPE);
  result.assign_assume_new("data", JSON{data}, MCP_HASH_DATA);
  result.assign_assume_new("mimeType", JSON{mime_type}, MCP_HASH_MIME_TYPE);
  auto annotation_object{
      mcp_serialize_resource_annotations(version, annotations)};
  if (!annotation_object.empty()) {
    result.assign_assume_new("annotations", std::move(annotation_object),
                             MCP_HASH_ANNOTATIONS);
  }
  internal::require(internal::valid_content(version, result),
                    "Invalid media block");
  return result;
}
} // namespace

auto mcp_make_image_block(const MCPProtocolVersion version,
                          const JSON::StringView data,
                          const JSON::StringView mime_type,
                          const MCPResourceAnnotations &annotations) -> JSON {
  return media_block(version, "image", data, mime_type, annotations);
}
auto mcp_make_audio_block(const MCPProtocolVersion version,
                          const JSON::StringView data,
                          const JSON::StringView mime_type,
                          const MCPResourceAnnotations &annotations) -> JSON {
  return media_block(version, "audio", data, mime_type, annotations);
}
auto mcp_make_embedded_resource(const MCPProtocolVersion version, JSON resource,
                                const MCPResourceAnnotations &annotations)
    -> JSON {
  internal::require(internal::valid_resource(version, resource, true, false),
                    "Invalid embedded resource");
  auto result{JSON::make_object()};
  result.assign_assume_new("type", JSON{"resource"}, MCP_HASH_TYPE);
  result.assign_assume_new("resource", std::move(resource), MCP_HASH_RESOURCE);
  auto annotation_object{
      mcp_serialize_resource_annotations(version, annotations)};
  if (!annotation_object.empty()) {
    result.assign_assume_new("annotations", std::move(annotation_object),
                             MCP_HASH_ANNOTATIONS);
  }
  internal::require(internal::valid_content(version, result),
                    "Invalid resource block");
  return result;
}
auto mcp_make_resource_blob_content(const JSON::StringView uri,
                                    const JSON::StringView mime_type,
                                    const JSON::StringView blob) -> JSON {
  require_resource_contents(uri, mime_type);
  auto result{JSON::make_object()};
  result.assign_assume_new("uri", JSON{uri}, MCP_HASH_URI);
  if (!mime_type.empty()) {
    result.assign_assume_new("mimeType", JSON{mime_type}, MCP_HASH_MIME_TYPE);
  }
  result.assign_assume_new("blob", JSON{blob}, MCP_HASH_BLOB);
  return result;
}

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
                            const JSON::StringView name,
                            const JSON::StringView mime_type,
                            const JSON::StringView description)
    -> sourcemeta::core::JSON {
  internal::require(mcp_supports_resource_link_content(version),
                    "Resource links require MCP 2025-06-18 or later");

  auto block{sourcemeta::core::JSON::make_object()};
  block.assign_assume_new("type", sourcemeta::core::JSON{"resource_link"},
                          MCP_HASH_TYPE);
  block.assign_assume_new("uri", sourcemeta::core::JSON{uri}, MCP_HASH_URI);
  block.assign_assume_new("name", sourcemeta::core::JSON{name}, MCP_HASH_NAME);
  if (!description.empty()) {
    block.assign_assume_new("description", sourcemeta::core::JSON{description},
                            MCP_HASH_DESCRIPTION);
  }
  if (!mime_type.empty()) {
    block.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                            MCP_HASH_MIME_TYPE);
  }
  return block;
}

namespace {
// The specification asks a server to identify itself on every result, so this
// is reachable from the interim and subscription results too, whose own result
// type rules out the complete-result decorator
// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#responses
auto attach_server_info(sourcemeta::core::JSON &result,
                        sourcemeta::core::JSON &&info) -> void {
  if (auto *meta{result.try_at("_meta", MCP_HASH_META)};
      meta != nullptr && meta->is_object()) {
    // Caller-supplied metadata may already identify the server
    meta->assign("io.modelcontextprotocol/serverInfo", std::move(info));
  } else {
    auto meta_object{sourcemeta::core::JSON::make_object()};
    meta_object.assign_assume_new("io.modelcontextprotocol/serverInfo",
                                  std::move(info), MCP_HASH_META_SERVER_INFO);
    result.assign_assume_new("_meta", std::move(meta_object), MCP_HASH_META);
  }
}

auto attach_server_info(const MCPProtocolVersion version,
                        sourcemeta::core::JSON &result,
                        const std::optional<MCPImplementation> &server_info)
    -> void {
  if (!server_info.has_value() || !mcp_requires_result_type(version)) {
    return;
  }
  attach_server_info(result,
                     internal::make_implementation(version, *server_info));
}

auto decorate_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON &result,
    const std::optional<MCPImplementation> &server_info = std::nullopt)
    -> void {
  internal::require(result.is_object(), "MCP results must be objects");
  internal::require(internal::valid_meta(result),
                    "MCP result _meta must be an object");
  if (!mcp_requires_result_type(version)) {
    return;
  }
  if (const auto *meta{result.try_at("_meta", sourcemeta::core::MCP_HASH_META)};
      meta) {
    internal::require(internal::valid_metadata_object(*meta, false),
                      "Invalid MCP 2026-07-28 metadata");
    if (const auto *info{
            meta->try_at("io.modelcontextprotocol/serverInfo",
                         sourcemeta::core::MCP_HASH_META_SERVER_INFO)};
        info) {
      internal::require(internal::valid_implementation(version, *info),
                        "Invalid server information");
    }
  }
  if (const auto *type{result.try_at("resultType", MCP_HASH_RESULT_TYPE)};
      type) {
    internal::require(type->is_string() && type->to_string() == "complete",
                      "The complete-result decorator cannot construct interim "
                      "or extension results");
  }

  auto info{
      server_info.has_value()
          ? std::optional<sourcemeta::core::JSON>{internal::make_implementation(
                version, *server_info)}
          : std::nullopt};

  if (result.try_at("resultType", MCP_HASH_RESULT_TYPE) == nullptr) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }

  if (info) {
    attach_server_info(result, std::move(*info));
  }
}

} // namespace

auto mcp_decorate_result(const MCPProtocolVersion version,
                         sourcemeta::core::JSON result,
                         const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  decorate_result(version, result, server_info);
  return result;
}

namespace {
auto decorate_cacheable_result(const MCPProtocolVersion version,
                               sourcemeta::core::JSON &result,
                               const MCPCachePolicy &cache_policy,
                               const JSON::StringView method) -> void {
  internal::require(result.is_object(), "MCP results must be objects");
  if (!mcp_requires_cacheable_metadata(version)) {
    return;
  }
  internal::require(mcp_is_cacheable_method(method),
                    "Operation is not cacheable");
  decorate_result(version, result, std::nullopt);
  const auto *result_type{result.try_at("resultType", MCP_HASH_RESULT_TYPE)};
  internal::require((result_type != nullptr) && result_type->is_string() &&
                        result_type->to_string() == "complete",
                    "Only complete results are cacheable");
  internal::require(cache_policy.ttl_ms >= 0,
                    "A cache time to live cannot be negative");
  // An already decorated result carries both hints, which this policy replaces
  result.assign("ttlMs", sourcemeta::core::JSON{cache_policy.ttl_ms});
  result.assign("cacheScope", sourcemeta::core::JSON{
                                  mcp_cache_scope_string(cache_policy.scope)});
}

} // namespace

auto mcp_decorate_cacheable_result(const MCPProtocolVersion version,
                                   sourcemeta::core::JSON result,
                                   const MCPCachePolicy &cache_policy,
                                   const JSON::StringView method)
    -> sourcemeta::core::JSON {
  decorate_cacheable_result(version, result, cache_policy, method);
  return result;
}

auto mcp_make_empty_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  return checked_success(
      identifier, mcp_decorate_result(version, std::move(result), server_info));
}

auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON result,
                           const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  internal::require(!mcp_supports_structured_content(version) ||
                        version == MCPProtocolVersion::V_2026_07_28 ||
                        result.is_object(),
                    "Legacy structuredContent must be an object");
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
  return checked_success(
      identifier,
      mcp_decorate_result(version, std::move(envelope_result), server_info));
}

auto mcp_make_tool_success(const MCPProtocolVersion version,
                           const sourcemeta::core::JSON &identifier,
                           sourcemeta::core::JSON structured,
                           sourcemeta::core::JSON content_blocks,
                           const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  internal::require(!mcp_supports_structured_content(version) ||
                        version == MCPProtocolVersion::V_2026_07_28 ||
                        structured.is_object(),
                    "Legacy structuredContent must be an object");
  require_array(content_blocks);
  for (const auto &block : content_blocks.as_array()) {
    internal::require(internal::valid_content(version, block),
                      "Invalid MCP content block");
  }
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
  return checked_success(
      identifier,
      mcp_decorate_result(version, std::move(envelope_result), server_info));
}

auto mcp_make_tool_error(const MCPProtocolVersion version,
                         const sourcemeta::core::JSON &identifier,
                         const JSON::StringView message,
                         const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  auto content{sourcemeta::core::JSON::make_array()};
  content.push_back(mcp_make_text_block(message));

  auto envelope_result{sourcemeta::core::JSON::make_object()};
  envelope_result.assign_assume_new("content", std::move(content),
                                    MCP_HASH_CONTENT);
  envelope_result.assign_assume_new("isError", sourcemeta::core::JSON{true},
                                    MCP_HASH_IS_ERROR);
  return checked_success(
      identifier,
      mcp_decorate_result(version, std::move(envelope_result), server_info));
}

auto mcp_make_resource(const MCPProtocolVersion version,
                       const JSON::StringView uri, const JSON::StringView name,
                       const JSON::StringView mime_type,
                       const JSON::StringView description,
                       const std::optional<std::size_t> size,
                       const std::optional<double> priority,
                       const MCPDescriptorPresentation &presentation,
                       const MCPResourceAnnotations &annotations)
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
  if (!mime_type.empty()) {
    resource.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                               MCP_HASH_MIME_TYPE);
  }
  if (size.has_value()) {
    resource.assign_assume_new("size", sourcemeta::core::JSON{size.value()},
                               MCP_HASH_SIZE);
  }
  auto combined{annotations};
  if (priority.has_value()) {
    combined.priority = priority;
  }
  auto annotation_object{mcp_serialize_resource_annotations(version, combined)};
  if (!annotation_object.empty()) {
    resource.assign_assume_new("annotations", std::move(annotation_object),
                               MCP_HASH_ANNOTATIONS);
  }
  apply_presentation(version, resource, presentation);
  internal::require(internal::valid_resource(version, resource, false, false),
                    "Invalid resource descriptor");
  return resource;
}

auto mcp_make_resource_text_content(const JSON::StringView uri,
                                    const JSON::StringView mime_type,
                                    const JSON::StringView text)
    -> sourcemeta::core::JSON {
  require_resource_contents(uri, mime_type);
  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("uri", sourcemeta::core::JSON{uri}, MCP_HASH_URI);
  if (!mime_type.empty()) {
    entry.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                            MCP_HASH_MIME_TYPE);
  }
  entry.assign_assume_new("text", sourcemeta::core::JSON{text}, MCP_HASH_TEXT);
  return entry;
}

auto mcp_make_resources_read_result(const MCPProtocolVersion version,
                                    sourcemeta::core::JSON contents,
                                    const MCPCachePolicy &cache_policy)
    -> sourcemeta::core::JSON {
  require_array(contents);
  for (const auto &entry : contents.as_array()) {
    internal::require(internal::valid_resource(version, entry, true, false),
                      "Invalid MCP resource contents entry");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  if (version == MCPProtocolVersion::V_2026_07_28) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }
  result.assign_assume_new("contents", std::move(contents), MCP_HASH_CONTENTS);
  return mcp_decorate_cacheable_result(version, std::move(result), cache_policy,
                                       MCP_METHOD_RESOURCES_READ);
}

auto mcp_make_tools_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON tools,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON {
  require_array(tools);
  for (const auto &entry : tools.as_array()) {
    internal::require(internal::valid_tool(version, entry),
                      "Invalid MCP tool descriptor entry");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  if (version == MCPProtocolVersion::V_2026_07_28) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }
  result.assign_assume_new("tools", std::move(tools), MCP_HASH_TOOLS);
  if (next_cursor.has_value()) {
    result.assign_assume_new("nextCursor",
                             sourcemeta::core::JSON{next_cursor.value()},
                             MCP_HASH_NEXT_CURSOR);
  }
  return mcp_decorate_cacheable_result(version, std::move(result), cache_policy,
                                       MCP_METHOD_TOOLS_LIST);
}

auto mcp_make_resources_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON resources,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON {
  require_array(resources);
  for (const auto &entry : resources.as_array()) {
    internal::require(internal::valid_resource(version, entry, false, false),
                      "Invalid MCP resource descriptor entry");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  if (version == MCPProtocolVersion::V_2026_07_28) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }
  result.assign_assume_new("resources", std::move(resources),
                           MCP_HASH_RESOURCES);
  if (next_cursor.has_value()) {
    result.assign_assume_new("nextCursor",
                             sourcemeta::core::JSON{next_cursor.value()},
                             MCP_HASH_NEXT_CURSOR);
  }
  return mcp_decorate_cacheable_result(version, std::move(result), cache_policy,
                                       MCP_METHOD_RESOURCES_LIST);
}

auto mcp_make_resource_templates_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON resource_templates,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON {
  require_array(resource_templates);
  for (const auto &entry : resource_templates.as_array()) {
    internal::require(internal::valid_resource(version, entry, false, true),
                      "Invalid MCP resource template entry");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  if (version == MCPProtocolVersion::V_2026_07_28) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }
  result.assign_assume_new("resourceTemplates", std::move(resource_templates),
                           MCP_HASH_RESOURCE_TEMPLATES);
  if (next_cursor.has_value()) {
    result.assign_assume_new("nextCursor",
                             sourcemeta::core::JSON{next_cursor.value()},
                             MCP_HASH_NEXT_CURSOR);
  }
  return mcp_decorate_cacheable_result(version, std::move(result), cache_policy,
                                       MCP_METHOD_RESOURCES_TEMPLATES_LIST);
}

auto mcp_make_prompts_list_result(
    const MCPProtocolVersion version, sourcemeta::core::JSON prompts,
    const std::optional<JSON::StringView> next_cursor,
    const MCPCachePolicy &cache_policy) -> sourcemeta::core::JSON {
  require_array(prompts);
  for (const auto &entry : prompts.as_array()) {
    internal::require(internal::valid_prompt(version, entry),
                      "Invalid MCP prompt descriptor entry");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  if (version == MCPProtocolVersion::V_2026_07_28) {
    result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                             MCP_HASH_RESULT_TYPE);
  }
  result.assign_assume_new("prompts", std::move(prompts), MCP_HASH_PROMPTS);
  if (next_cursor.has_value()) {
    result.assign_assume_new("nextCursor",
                             sourcemeta::core::JSON{next_cursor.value()},
                             MCP_HASH_NEXT_CURSOR);
  }
  return mcp_decorate_cacheable_result(version, std::move(result), cache_policy,
                                       MCP_METHOD_PROMPTS_LIST);
}

auto mcp_make_resource_template(const MCPProtocolVersion version,
                                const JSON::StringView uri_template,
                                const JSON::StringView name,
                                const JSON::StringView description,
                                const JSON::StringView mime_type,
                                const MCPDescriptorPresentation &presentation,
                                const MCPResourceAnnotations &annotations)
    -> sourcemeta::core::JSON {
  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("uriTemplate", sourcemeta::core::JSON{uri_template},
                          MCP_HASH_URI_TEMPLATE);
  entry.assign_assume_new("name", sourcemeta::core::JSON{name}, MCP_HASH_NAME);
  if (!description.empty()) {
    entry.assign_assume_new("description", sourcemeta::core::JSON{description},
                            MCP_HASH_DESCRIPTION);
  }
  if (!mime_type.empty()) {
    entry.assign_assume_new("mimeType", sourcemeta::core::JSON{mime_type},
                            MCP_HASH_MIME_TYPE);
  }
  apply_presentation(version, entry, presentation);
  auto annotation_object{
      mcp_serialize_resource_annotations(version, annotations)};
  if (!annotation_object.empty()) {
    entry.assign_assume_new("annotations", std::move(annotation_object),
                            MCP_HASH_ANNOTATIONS);
  }
  internal::require(internal::valid_resource(version, entry, false, true),
                    "Invalid resource template");
  return entry;
}

auto mcp_make_tool_descriptor(
    const MCPProtocolVersion version, const JSON::StringView name,
    const JSON::StringView description, sourcemeta::core::JSON input_schema,
    std::optional<sourcemeta::core::JSON> output_schema,
    const MCPToolAnnotations &annotations,
    const MCPDescriptorPresentation &presentation) -> sourcemeta::core::JSON {

  auto entry{sourcemeta::core::JSON::make_object()};
  entry.assign_assume_new("name", sourcemeta::core::JSON{name}, MCP_HASH_NAME);
  if (!description.empty()) {
    entry.assign_assume_new("description", sourcemeta::core::JSON{description},
                            MCP_HASH_DESCRIPTION);
  }
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

  apply_presentation(version, entry, presentation);
  internal::require(internal::valid_tool(version, entry),
                    "Invalid MCP tool descriptor");
  return entry;
}

auto mcp_make_initialize_response(
    const sourcemeta::core::JSON &request,
    const MCPServerCapabilities &capabilities, const MCPImplementation &server,
    const std::span<const JSON::StringView> supported_versions,
    const JSON::StringView instructions) -> sourcemeta::core::JSON {
  internal::require(!supported_versions.empty(),
                    "Supported versions must be explicit");
  std::optional<MCPProtocolVersion> chosen;
  for (const auto wire : supported_versions) {
    const auto revision{mcp_resolve_protocol_version(wire)};
    internal::require(revision.has_value(),
                      "Unknown server-supported MCP version");
    if (mcp_uses_initialization_handshake(*revision) &&
        (!chosen || mcp_protocol_version_at_least(*revision, *chosen))) {
      chosen = revision;
    }
  }
  internal::require(chosen.has_value(),
                    "No initialization-compatible version is supported");
  const auto *identifier{jsonrpc_request_id(request)};
  const auto *parameters{jsonrpc_params(request)};
  if ((identifier == nullptr) || !internal::valid_id(*identifier) ||
      !jsonrpc_is_request(request) ||
      jsonrpc_method(request) != MCP_METHOD_INITIALIZE ||
      (parameters == nullptr) || !parameters->is_object()) {
    return mcp_make_error(*chosen, identifier, JSONRPC_CODE_INVALID_REQUEST,
                          "Invalid Request");
  }
  const auto *protocol_version_field{
      parameters->try_at("protocolVersion", MCP_HASH_PROTOCOL_VERSION)};
  const auto *client_info{
      parameters->try_at("clientInfo", sourcemeta::core::MCP_HASH_CLIENT_INFO)};
  const auto *client_capabilities{parameters->try_at(
      "capabilities", sourcemeta::core::MCP_HASH_CAPABILITIES)};
  if ((protocol_version_field == nullptr) ||
      !protocol_version_field->is_string() || (client_info == nullptr) ||
      (client_capabilities == nullptr)) {
    return mcp_make_error(*chosen, identifier, JSONRPC_CODE_INVALID_PARAMS,
                          "Invalid params");
  }
  const auto requested{
      mcp_resolve_protocol_version(protocol_version_field->to_string())};
  if (requested && mcp_uses_initialization_handshake(*requested) &&
      std::find(supported_versions.begin(), supported_versions.end(),
                protocol_version_field->to_string()) !=
          supported_versions.end()) {
    chosen = requested;
  }
  const auto version{*chosen};
  if (!internal::valid_implementation(version, *client_info, true) ||
      !internal::valid_capabilities(version, *client_capabilities, true)) {
    return mcp_make_error(version, identifier, JSONRPC_CODE_INVALID_PARAMS,
                          "Invalid params");
  }
  auto capabilities_object{
      mcp_serialize_server_capabilities(version, capabilities)};

  auto server_info{internal::make_implementation(version, server)};

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
  return checked_success(*identifier, std::move(result));
}

auto mcp_make_server_discover_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const MCPServerCapabilities &capabilities, const MCPImplementation &server,
    const std::span<const JSON::StringView> supported_versions,
    const JSON::StringView instructions, const MCPCachePolicy &cache_policy)
    -> sourcemeta::core::JSON {
  internal::require(version == MCPProtocolVersion::V_2026_07_28,
                    "Discovery requires MCP 2026-07-28");
  internal::require(!supported_versions.empty(),
                    "Discovery must declare supported versions");
  internal::require(
      std::find(supported_versions.begin(), supported_versions.end(),
                MCP_PROTOCOL_VERSION_2026_07_28) != supported_versions.end(),
      "Discovery must include the serving revision");
  for (const auto wire : supported_versions) {
    internal::require(mcp_resolve_protocol_version(wire).has_value(),
                      "Unknown advertised protocol version");
  }
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                           MCP_HASH_RESULT_TYPE);

  auto versions_array{sourcemeta::core::JSON::make_array()};
  for (const auto supported_version : supported_versions) {
    versions_array.push_back(sourcemeta::core::JSON{supported_version});
  }
  result.assign_assume_new("supportedVersions", std::move(versions_array),
                           MCP_HASH_SUPPORTED_VERSIONS);

  result.assign_assume_new("capabilities",
                           mcp_serialize_server_capabilities(
                               MCPProtocolVersion::V_2026_07_28, capabilities),
                           MCP_HASH_CAPABILITIES);

  auto server_info{internal::make_implementation(version, server)};

  auto meta{sourcemeta::core::JSON::make_object()};
  meta.assign_assume_new("io.modelcontextprotocol/serverInfo",
                         std::move(server_info), MCP_HASH_META_SERVER_INFO);
  result.assign_assume_new("_meta", std::move(meta), MCP_HASH_META);

  if (!instructions.empty()) {
    result.assign_assume_new("instructions",
                             sourcemeta::core::JSON{instructions},
                             MCP_HASH_INSTRUCTIONS);
  }

  internal::require(cache_policy.ttl_ms >= 0,
                    "A cache time to live cannot be negative");
  result.assign_assume_new("ttlMs", sourcemeta::core::JSON{cache_policy.ttl_ms},
                           MCP_HASH_TTL_MS);
  result.assign_assume_new(
      "cacheScope",
      sourcemeta::core::JSON{mcp_cache_scope_string(cache_policy.scope)},
      MCP_HASH_CACHE_SCOPE);

  return checked_success(identifier, std::move(result));
}

auto mcp_make_input_required_response(
    const MCPProtocolVersion version, const JSON::StringView method,
    const sourcemeta::core::JSON &identifier,
    std::optional<sourcemeta::core::JSON> input_requests,
    std::optional<JSON::StringView> request_state,
    const MCPClientCapabilities &client_capabilities,
    const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  internal::require(mcp_supports_mrtr(version), "MRTR requires MCP 2026-07-28");
  internal::require(mcp_is_mrtr_method(method),
                    "Operation does not support MRTR");
  internal::require(input_requests.has_value() || request_state.has_value(),
                    "An interim result requires input or state");
  if (input_requests) {
    internal::require(
        internal::valid_input_requests(*input_requests, client_capabilities),
        "Invalid or unsupported nested input requests");
  }

  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new("resultType",
                           sourcemeta::core::JSON{"input_required"},
                           MCP_HASH_RESULT_TYPE);

  if (input_requests.has_value()) {
    result.assign_assume_new("inputRequests", std::move(input_requests).value(),
                             MCP_HASH_INPUT_REQUESTS);
  }

  if (request_state.has_value()) {
    result.assign_assume_new("requestState",
                             sourcemeta::core::JSON{request_state.value()},
                             MCP_HASH_REQUEST_STATE);
  }

  attach_server_info(version, result, server_info);
  return checked_success(identifier, std::move(result));
}

auto mcp_request_input_responses(const MCPProtocolVersion version,
                                 const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  const auto *request_id{jsonrpc_request_id(envelope)};
  if ((request_id == nullptr) || !internal::valid_id(*request_id)) {
    return nullptr;
  }

  if (!mcp_supports_mrtr(version) || !jsonrpc_is_request(envelope) ||
      !mcp_is_mrtr_method(jsonrpc_method(envelope))) {
    return nullptr;
  }
  const auto *parameters{sourcemeta::core::jsonrpc_params(envelope)};
  if (parameters == nullptr || !parameters->is_object()) {
    return nullptr;
  }
  const auto *input_responses_field{
      parameters->try_at("inputResponses", MCP_HASH_INPUT_RESPONSES)};
  if (input_responses_field == nullptr || !input_responses_field->is_object()) {
    return nullptr;
  }
  return input_responses_field;
}

auto mcp_request_state(const MCPProtocolVersion version,
                       const sourcemeta::core::JSON &envelope)
    -> std::optional<JSON::StringView> {
  const auto *request_id{jsonrpc_request_id(envelope)};
  if ((request_id == nullptr) || !internal::valid_id(*request_id)) {
    return std::nullopt;
  }

  if (!mcp_supports_mrtr(version) || !jsonrpc_is_request(envelope) ||
      !mcp_is_mrtr_method(jsonrpc_method(envelope))) {
    return std::nullopt;
  }
  const auto *parameters{sourcemeta::core::jsonrpc_params(envelope)};
  if (parameters == nullptr || !parameters->is_object()) {
    return std::nullopt;
  }
  const auto *request_state_field{
      parameters->try_at("requestState", MCP_HASH_REQUEST_STATE)};
  if (request_state_field == nullptr || !request_state_field->is_string()) {
    return std::nullopt;
  }
  return request_state_field->to_string();
}

auto mcp_make_subscription_acknowledged_notification(
    const MCPProtocolVersion version,
    const sourcemeta::core::JSON &subscription_id,
    const MCPSubscriptionFilter &requested,
    const MCPSubscriptionFilter &supported) -> sourcemeta::core::JSON {
  internal::require(mcp_supports_subscriptions_listen(version),
                    "Subscriptions require MCP 2026-07-28");
  internal::require(internal::valid_id(subscription_id),
                    "Invalid subscription identifier");
  const auto notifications{
      mcp_intersect_subscription_filters(requested, supported)};
  auto filter{JSON::make_object()};
  if (notifications.tools_list_changed) {
    filter.assign_assume_new("toolsListChanged",
                             JSON{*notifications.tools_list_changed},
                             sourcemeta::core::MCP_HASH_TOOLS_LIST_CHANGED);
  }
  if (notifications.prompts_list_changed) {
    filter.assign_assume_new("promptsListChanged",
                             JSON{*notifications.prompts_list_changed},
                             sourcemeta::core::MCP_HASH_PROMPTS_LIST_CHANGED);
  }
  if (notifications.resources_list_changed) {
    filter.assign_assume_new("resourcesListChanged",
                             JSON{*notifications.resources_list_changed},
                             sourcemeta::core::MCP_HASH_RESOURCES_LIST_CHANGED);
  }
  if (notifications.resource_subscriptions) {
    auto resources{JSON::make_array()};
    for (const auto uri : *notifications.resource_subscriptions) {
      resources.push_back(JSON{uri});
    }
    filter.assign_assume_new("resourceSubscriptions", std::move(resources),
                             sourcemeta::core::MCP_HASH_RESOURCE_SUBSCRIPTIONS);
  }
  auto meta{sourcemeta::core::JSON::make_object()};
  meta.assign_assume_new("io.modelcontextprotocol/subscriptionId",
                         sourcemeta::core::JSON{subscription_id},
                         MCP_HASH_META_SUBSCRIPTION_ID);

  auto parameters{sourcemeta::core::JSON::make_object()};
  parameters.assign_assume_new("_meta", std::move(meta), MCP_HASH_META);
  parameters.assign_assume_new("notifications", std::move(filter),
                               MCP_HASH_NOTIFICATIONS);

  auto notification{sourcemeta::core::JSON::make_object()};
  notification.assign_assume_new("jsonrpc", sourcemeta::core::JSON{"2.0"},
                                 MCP_HASH_JSONRPC);
  notification.assign_assume_new(
      "method",
      sourcemeta::core::JSON{
          MCP_METHOD_NOTIFICATIONS_SUBSCRIPTIONS_ACKNOWLEDGED},
      MCP_HASH_METHOD);
  notification.assign_assume_new("params", std::move(parameters),
                                 MCP_HASH_PARAMS);
  return notification;
}

auto mcp_make_subscription_acknowledged_notification(
    const MCPProtocolVersion version, const JSON::StringView subscription_id,
    const MCPSubscriptionFilter &requested,
    const MCPSubscriptionFilter &supported) -> sourcemeta::core::JSON {
  return mcp_make_subscription_acknowledged_notification(
      version, sourcemeta::core::JSON{subscription_id}, requested, supported);
}

auto mcp_make_subscription_close_response(
    const MCPProtocolVersion version, const sourcemeta::core::JSON &identifier,
    const std::optional<MCPImplementation> &server_info)
    -> sourcemeta::core::JSON {
  internal::require(mcp_supports_subscriptions_listen(version),
                    "Subscriptions require MCP 2026-07-28");
  internal::require(internal::valid_id(identifier),
                    "Invalid subscription identifier");
  auto meta{sourcemeta::core::JSON::make_object()};
  meta.assign_assume_new("io.modelcontextprotocol/subscriptionId",
                         sourcemeta::core::JSON{identifier},
                         MCP_HASH_META_SUBSCRIPTION_ID);

  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new("resultType", sourcemeta::core::JSON{"complete"},
                           MCP_HASH_RESULT_TYPE);
  result.assign_assume_new("_meta", std::move(meta), MCP_HASH_META);

  attach_server_info(version, result, server_info);
  return checked_success(identifier, std::move(result));
}

auto mcp_request_subscription_id(const MCPProtocolVersion version,
                                 const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  if (!mcp_supports_subscriptions_listen(version)) {
    return nullptr;
  }
  if (!envelope.is_object()) {
    return nullptr;
  }
  const sourcemeta::core::JSON *meta = nullptr;
  if (const auto *parameters{envelope.try_at("params", MCP_HASH_PARAMS)};
      parameters != nullptr && parameters->is_object()) {
    meta = parameters->try_at("_meta", MCP_HASH_META);
  } else if (const auto *result{envelope.try_at("result", MCP_HASH_RESULT)};
             result != nullptr && result->is_object()) {
    meta = result->try_at("_meta", MCP_HASH_META);
  } else {
    meta = envelope.try_at("_meta", MCP_HASH_META);
  }

  if (meta == nullptr || !meta->is_object()) {
    return nullptr;
  }

  const auto *sub_id{meta->try_at("io.modelcontextprotocol/subscriptionId",
                                  MCP_HASH_META_SUBSCRIPTION_ID)};
  if (sub_id == nullptr || !internal::valid_id(*sub_id)) {
    return nullptr;
  }
  return sub_id;
}

auto mcp_tool_call_arguments(const sourcemeta::core::JSON &envelope)
    -> const sourcemeta::core::JSON * {
  const auto *request_id{jsonrpc_request_id(envelope)};
  if ((request_id == nullptr) || !internal::valid_id(*request_id)) {
    return nullptr;
  }

  if (!jsonrpc_is_request(envelope) ||
      jsonrpc_method(envelope) != MCP_METHOD_TOOLS_CALL) {
    return nullptr;
  }
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

auto mcp_make_prompts_get_result(const MCPProtocolVersion version,
                                 const JSON::StringView description,
                                 JSON messages) -> JSON {
  require_array(messages);
  for (const auto &message : messages.as_array()) {
    internal::require(message.is_object(), "Prompt messages must be objects");
    const auto *role{message.try_at("role", sourcemeta::core::MCP_HASH_ROLE)};
    const auto *content{
        message.try_at("content", sourcemeta::core::MCP_HASH_CONTENT)};
    internal::require(
        (role != nullptr) && role->is_string() &&
            (role->to_string() == "user" || role->to_string() == "assistant") &&
            (content != nullptr) && internal::valid_content(version, *content),
        "Invalid prompt message");
  }
  auto result{JSON::make_object()};
  if (!description.empty()) {
    result.assign_assume_new("description", JSON{description},
                             sourcemeta::core::MCP_HASH_DESCRIPTION);
  }
  result.assign_assume_new("messages", std::move(messages),
                           sourcemeta::core::MCP_HASH_MESSAGES);
  decorate_result(version, result);
  return result;
}

auto mcp_make_completion_result(const MCPProtocolVersion version, JSON values,
                                const std::optional<std::int64_t> total,
                                const std::optional<bool> has_more) -> JSON {
  require_array(values);
  internal::require(values.size() <= 100,
                    "At most 100 completion values are permitted");
  for (const auto &value : values.as_array()) {
    internal::require(value.is_string(), "Completion values must be strings");
  }
  internal::require(
      !total || (*total >= 0 && std::cmp_greater_equal(*total, values.size())),
      "Invalid completion total");
  auto completion{JSON::make_object()};
  completion.assign_assume_new("values", std::move(values),
                               sourcemeta::core::MCP_HASH_VALUES);
  if (total) {
    completion.assign_assume_new("total", JSON{*total},
                                 sourcemeta::core::MCP_HASH_TOTAL);
  }
  if (has_more) {
    completion.assign_assume_new("hasMore", JSON{*has_more},
                                 sourcemeta::core::MCP_HASH_HAS_MORE);
  }
  auto result{JSON::make_object()};
  result.assign_assume_new("completion", std::move(completion),
                           sourcemeta::core::MCP_HASH_COMPLETION);
  decorate_result(version, result);
  return result;
}

namespace {
auto validate_write_result(const MCPProtocolVersion version,
                           const JSON::StringView method,
                           const JSON &identifier, const JSON &result,
                           const std::optional<MCPCachePolicy> &cache_policy)
    -> void {
  internal::require(internal::valid_id(identifier), "Invalid MCP identifier");
  internal::require(mcp_is_request_method(version, method) &&
                        mcp_is_cacheable_method(method),
                    "Writer requires a cacheable operation");
  internal::require(result.is_object() && internal::valid_meta(result),
                    "Invalid MCP result object");
  if (version == MCPProtocolVersion::V_2026_07_28) {
    if (const auto *meta{
            result.try_at("_meta", sourcemeta::core::MCP_HASH_META)};
        meta) {
      internal::require(internal::valid_metadata_object(*meta, false),
                        "Invalid precomputed metadata");
      if (const auto *info{
              meta->try_at("io.modelcontextprotocol/serverInfo",
                           sourcemeta::core::MCP_HASH_META_SERVER_INFO)};
          info) {
        internal::require(internal::valid_implementation(version, *info),
                          "Invalid precomputed server information");
      }
    }
  }
  const auto *type{
      result.try_at("resultType", sourcemeta::core::MCP_HASH_RESULT_TYPE)};
  internal::require((type == nullptr) ||
                        (type->is_string() && type->to_string() == "complete"),
                    "Writer requires a complete result");
  if (version == MCPProtocolVersion::V_2026_07_28) {
    internal::require(cache_policy.has_value(),
                      "Cacheable results require a policy");
  }
  if (cache_policy) {
    internal::require(cache_policy->ttl_ms >= 0,
                      "A cache time to live cannot be negative");
  }
  const JSON *entries = nullptr;
  if (method == MCP_METHOD_TOOLS_LIST) {
    entries = result.try_at("tools", sourcemeta::core::MCP_HASH_TOOLS);
  } else if (method == MCP_METHOD_RESOURCES_LIST) {
    entries = result.try_at("resources", sourcemeta::core::MCP_HASH_RESOURCES);
  } else if (method == MCP_METHOD_RESOURCES_TEMPLATES_LIST) {
    entries = result.try_at("resourceTemplates",
                            sourcemeta::core::MCP_HASH_RESOURCE_TEMPLATES);
  } else if (method == MCP_METHOD_PROMPTS_LIST) {
    entries = result.try_at("prompts", sourcemeta::core::MCP_HASH_PROMPTS);
  } else if (method == MCP_METHOD_RESOURCES_READ) {
    entries = result.try_at("contents", sourcemeta::core::MCP_HASH_CONTENTS);
  } else {
    const auto *versions{result.try_at(
        "supportedVersions", sourcemeta::core::MCP_HASH_SUPPORTED_VERSIONS)};
    const auto *capabilities{
        result.try_at("capabilities", sourcemeta::core::MCP_HASH_CAPABILITIES)};
    internal::require(
        (versions != nullptr) && versions->is_array() && !versions->empty() &&
            (capabilities != nullptr) &&
            internal::valid_capabilities(version, *capabilities, false),
        "Invalid discovery result");
    internal::require(
        !result.defines("instructions",
                        sourcemeta::core::MCP_HASH_INSTRUCTIONS) ||
            result.at("instructions", sourcemeta::core::MCP_HASH_INSTRUCTIONS)
                .is_string(),
        "Invalid discovery instructions");
    bool serving_revision = false;
    for (const auto &wire : versions->as_array()) {
      internal::require(
          wire.is_string() &&
              mcp_resolve_protocol_version(wire.to_string()).has_value(),
          "Invalid discovery protocol version");
      serving_revision =
          serving_revision ||
          wire.to_string() == mcp_protocol_version_string(version);
    }
    internal::require(serving_revision,
                      "Discovery must include its serving revision");
  }
  if (entries != nullptr) {
    require_array(*entries);
    for (const auto &entry : entries->as_array()) {
      const bool valid{
          method == MCP_METHOD_TOOLS_LIST ? internal::valid_tool(version, entry)
          : method == MCP_METHOD_PROMPTS_LIST
              ? internal::valid_prompt(version, entry)
              : internal::valid_resource(
                    version, entry, method == MCP_METHOD_RESOURCES_READ,
                    method == MCP_METHOD_RESOURCES_TEMPLATES_LIST)};
      internal::require(valid, "Invalid precomputed MCP result entry");
    }
  } else {
    internal::require(method == MCP_METHOD_SERVER_DISCOVER,
                      "Missing precomputed result entries");
  }
  if (const auto *cursor{
          result.try_at("nextCursor", sourcemeta::core::MCP_HASH_NEXT_CURSOR)};
      cursor) {
    internal::require(cursor->is_string(), "Invalid result cursor");
  }
}

auto write_result_impl(std::ostream &stream, const MCPProtocolVersion version,
                       const JSON::StringView method, const JSON &identifier,
                       JSON &&result,
                       const std::optional<MCPCachePolicy> &cache_policy,
                       const std::optional<MCPImplementation> &server_info)
    -> void {
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  const auto envelope_identifier{identifier};
  const JSON::String operation{method};
  decorate_result(version, result, server_info);
  if (cache_policy) {
    decorate_cacheable_result(version, result, *cache_policy, operation);
  }
  stringify(jsonrpc_make_success(envelope_identifier, std::move(result)),
            stream);
}
} // namespace

auto mcp_write_result(std::ostream &stream, const MCPProtocolVersion version,
                      const JSON::StringView method, const JSON &identifier,
                      JSON &&result,
                      const std::optional<MCPCachePolicy> cache_policy,
                      const std::optional<MCPImplementation> server_info)
    -> void {
  validate_write_result(version, method, identifier, result, cache_policy);
  write_result_impl(stream, version, method, identifier, std::move(result),
                    cache_policy, server_info);
}

auto mcp_write_result(std::ostream &stream, const MCPProtocolVersion version,
                      const JSON::StringView method, const JSON &identifier,
                      const JSON &result,
                      const std::optional<MCPCachePolicy> cache_policy,
                      const std::optional<MCPImplementation> server_info)
    -> void {
  validate_write_result(version, method, identifier, result, cache_policy);
  auto payload{result};
  write_result_impl(stream, version, method, identifier, std::move(payload),
                    cache_policy, server_info);
}
} // namespace sourcemeta::core
