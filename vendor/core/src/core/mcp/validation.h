#ifndef SOURCEMETA_CORE_MCP_VALIDATION_H_
#define SOURCEMETA_CORE_MCP_VALIDATION_H_

#include <sourcemeta/core/mcp.h>

#include "helpers.h"

#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>        // std::all_of
#include <cmath>            // std::isfinite
#include <initializer_list> // std::initializer_list
#include <set>              // std::set
#include <utility>          // std::move, std::pair

namespace sourcemeta::core::internal {

inline auto require(const bool condition, const char *message) -> void {
  if (!condition) {
    throw MCPError{message};
  }
}

inline auto valid_id(const JSON &identifier) noexcept -> bool {
  return identifier.is_string() || identifier.is_integral();
}

inline auto valid_implementation(MCPProtocolVersion version, const JSON &value,
                                 bool received = false) -> bool;
inline auto make_implementation(MCPProtocolVersion version,
                                const MCPImplementation &value) -> JSON;
inline auto valid_capabilities(MCPProtocolVersion version, const JSON &value,
                               bool client) -> bool;
inline auto valid_extensions(const JSON &value) -> bool;
inline auto valid_tool(MCPProtocolVersion version, const JSON &value) -> bool;
inline auto valid_content(MCPProtocolVersion version, const JSON &value,
                          bool received = false) -> bool;
inline auto valid_resource(MCPProtocolVersion version, const JSON &value,
                           bool contents, bool is_template,
                           bool received = false) -> bool;
inline auto valid_prompt(MCPProtocolVersion version, const JSON &value) -> bool;
inline auto valid_input_requests(const JSON &value,
                                 const MCPClientCapabilities &capabilities)
    -> bool;
inline auto valid_input_response(JSON::StringView method, const JSON &request,
                                 const JSON &value) -> bool;
inline auto valid_meta(const JSON &value) -> bool;
inline auto valid_meta(MCPProtocolVersion version, const JSON &value) -> bool;
inline auto valid_metadata_object(const JSON &meta, bool received) -> bool;
inline auto valid_log_level(JSON::StringView value) noexcept -> bool;

} // namespace sourcemeta::core::internal

namespace sourcemeta::core::internal::validation {
using sourcemeta::core::JSON;
using sourcemeta::core::MCPProtocolVersion;

inline auto field(const JSON &value, const JSON::StringView name,
                  const JSON::Object::hash_type hash) -> const JSON * {
  return value.is_object() ? value.try_at(name, hash) : nullptr;
}

inline auto string_field(const JSON &value, const JSON::StringView name,
                         const JSON::Object::hash_type hash,
                         const bool required = false) -> bool {
  const auto *item{field(value, name, hash)};
  return item == nullptr ? !required : item->is_string();
}

inline auto
optional_fields(const JSON &value,
                const std::initializer_list<
                    std::pair<JSON::StringView, JSON::Object::hash_type>>
                    names,
                const JSON::Type type) -> bool {
  return std::all_of(names.begin(), names.end(), [&](const auto name) {
    const auto *item{field(value, name.first, name.second)};
    return item == nullptr || item->type() == type;
  });
}

// The specification describes four string fields by naming an external format
// and then only recommending it, in prose rather than in capitals. These hold
// such a field to its format anyway, which is stricter than what is required.
//
// What makes the strictness safe is where it applies. A recommendation about a
// format is addressed to whoever writes the value, so holding a value this
// module builds to it refuses only what this module would have emitted, which
// costs no interoperability. A value arriving from a peer is a different
// matter, and refusing a handshake over the shape of an icon would cost a
// great deal, so the receive path asks only that such a field be a string.
//
// Almost every path reaching these checks builds a value, so the distinction
// needs carrying in one place only: an implementation descriptor is validated
// in both directions, and the flag it takes says which direction this is.
//
// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#general-fields

// "The moment the resource was last modified, as an ISO 8601 formatted string"
// with the example "2025-01-12T15:00:58Z". This holds it to RFC 3339, which is
// the profile of ISO 8601 that example is written in and the one that
// interoperates, rather than to the wider calendar formats ISO 8601 also
// admits
// https://www.rfc-editor.org/rfc/rfc3339
inline auto timestamp_field(const JSON &value, const JSON::StringView name,
                            const JSON::Object::hash_type hash) -> bool {
  const auto *item{value.try_at(name, hash)};
  return (item == nullptr) ||
         (item->is_string() &&
          sourcemeta::core::is_rfc3339_datetime(item->to_string()));
}

// A media type is the one the HTTP module already parses, since the examples
// the specification gives are ordinary ones such as "image/svg+xml"
inline auto media_type_field(const JSON &value, const JSON::StringView name,
                             const JSON::Object::hash_type hash,
                             const bool required = false) -> bool {
  const auto *item{value.try_at(name, hash)};
  if (item == nullptr) {
    return !required;
  }

  return item->is_string() &&
         sourcemeta::core::http_parse_media_type(item->to_string()).has_value();
}

// "A standard URI pointing to an icon resource", and for a resource the
// security considerations turn the same expectation into a requirement:
// "Servers MUST validate all resource URIs". A resource is addressed by scheme,
// which is why this asks for a URI rather than a reference to resolve
// https://modelcontextprotocol.io/specification/2026-07-28/schema#icon
// https://modelcontextprotocol.io/specification/2026-07-28/server/resources#security-considerations
inline auto uri_field(const JSON &value, const JSON::StringView name,
                      const JSON::Object::hash_type hash,
                      const bool required = false) -> bool {
  const auto *item{value.try_at(name, hash)};
  if (item == nullptr) {
    return !required;
  }

  return item->is_string() && sourcemeta::core::URI::is_uri(item->to_string());
}

// "Each string should be in WxH format (e.g., "48x48", "96x96") or "any" for
// scalable formats like SVG", so each is a pair of decimal counts or the one
// word that stands for every size
// https://modelcontextprotocol.io/specification/2026-07-28/schema#icon
inline auto icon_size(const JSON::StringView value) -> bool {
  if (value == "any") {
    return true;
  }

  const auto separator{value.find('x')};
  if (separator == JSON::StringView::npos || separator == 0 ||
      separator + 1 == value.size()) {
    return false;
  }

  // A pair, so exactly one separator with a run of decimal digits either side
  return value.find('x', separator + 1) == JSON::StringView::npos &&
         std::all_of(value.begin(), value.end(), [](const char character) {
           return sourcemeta::core::is_digit(character) || character == 'x';
         });
}

inline auto strings(const JSON &value) -> bool {
  return value.is_array() &&
         std::all_of(value.as_array().begin(), value.as_array().end(),
                     [](const auto &item) { return item.is_string(); });
}

inline auto annotations(const JSON &value, const bool received = false)
    -> bool {
  const auto *item{
      field(value, "annotations", sourcemeta::core::MCP_HASH_ANNOTATIONS)};
  if (item == nullptr) {
    return true;
  }
  if (!item->is_object() ||
      !(received ? string_field(*item, "lastModified",
                                sourcemeta::core::MCP_HASH_LAST_MODIFIED)
                 : timestamp_field(*item, "lastModified",
                                   sourcemeta::core::MCP_HASH_LAST_MODIFIED))) {
    return false;
  }
  if (const auto *priority{
          field(*item, "priority", sourcemeta::core::MCP_HASH_PRIORITY)};
      priority != nullptr &&
      (!priority->is_number() ||
       (priority->is_real() && !std::isfinite(priority->to_real())) ||
       *priority < JSON{0} || *priority > JSON{1})) {
    return false;
  }
  if (const auto *audience{
          field(*item, "audience", sourcemeta::core::MCP_HASH_AUDIENCE)};
      audience != nullptr) {
    if (!strings(*audience)) {
      return false;
    }
    for (const auto &role : audience->as_array()) {
      if (role.to_string() != "user" && role.to_string() != "assistant") {
        return false;
      }
    }
  }
  return true;
}

inline auto icons(const JSON &value, const bool received = false) -> bool {
  const auto *items{field(value, "icons", sourcemeta::core::MCP_HASH_ICONS)};
  if (items == nullptr) {
    return true;
  }
  if (!items->is_array()) {
    return false;
  }
  for (const auto &item : items->as_array()) {
    if (!item.is_object() ||
        !(received
              ? string_field(item, "src", sourcemeta::core::MCP_HASH_SRC, true)
              : uri_field(item, "src", sourcemeta::core::MCP_HASH_SRC, true)) ||
        !(received ? string_field(item, "mimeType",
                                  sourcemeta::core::MCP_HASH_MIME_TYPE)
                   : media_type_field(item, "mimeType",
                                      sourcemeta::core::MCP_HASH_MIME_TYPE))) {
      return false;
    }
    if (const auto *sizes{
            field(item, "sizes", sourcemeta::core::MCP_HASH_SIZES)};
        sizes != nullptr) {
      if (!strings(*sizes)) {
        return false;
      }

      for (const auto &size : sizes->as_array()) {
        if (!received && !icon_size(size.to_string())) {
          return false;
        }
      }
    }
    if (const auto *theme{
            field(item, "theme", sourcemeta::core::MCP_HASH_THEME)};
        (theme != nullptr) &&
        (!theme->is_string() ||
         (theme->to_string() != "light" && theme->to_string() != "dark"))) {
      return false;
    }
  }
  return true;
}

// Several MCP members carry a JSON Schema. This module checks such a member
// only as far as the specification constrains the member itself, which is that
// it is an object declaring the object type, and deliberately does not
// interpret the schema inside it.
//
// The two cases differ in why. A tool schema is declared with an open index
// signature, so beyond the object type the specification constrains nothing
// and there is nothing further to check. An elicitation form schema is
// restricted to a set of primitive property forms, which is a real constraint,
// but checking it is schema validation rather than wire-shape validation: it
// belongs to a schema evaluator layered on top, which also keeps this module
// from carrying a second implementation of rules that evaluator already owns.
//
// Keeping the line here means a caller can build a form whose properties this
// module never inspects. That is intended, and a test pins it.
// https://modelcontextprotocol.io/specification/2026-07-28/schema#tool
// https://modelcontextprotocol.io/specification/2026-07-28/schema#elicitrequestformparams
inline auto object_schema(const JSON &value) -> bool {
  const auto *type{field(value, "type", sourcemeta::core::MCP_HASH_TYPE)};
  return (type != nullptr) && type->is_string() &&
         type->to_string() == "object";
}

inline auto base(const JSON &value, const bool received = false) -> bool {
  return value.is_object() &&
         string_field(value, "name", sourcemeta::core::MCP_HASH_NAME, true) &&
         string_field(value, "title", sourcemeta::core::MCP_HASH_TITLE) &&
         string_field(value, "description",
                      sourcemeta::core::MCP_HASH_DESCRIPTION) &&
         sourcemeta::core::internal::valid_meta(value) &&
         icons(value, received);
}

// A key is an optional prefix followed by a name. The prefix reads like a
// reverse domain name and deliberately is not one, so this spells out its own
// rules rather than deferring to the host name predicate this project already
// offers.
//
// What the two have in common is the shape: dot separated labels of letters,
// digits and hyphens. Where they part is the first character of a label.
// Here, "Labels MUST start with a letter and end with a letter or digit",
// while host names were changed in 1989 so that "the restriction on the first
// character is relaxed to allow either a letter or a digit", in Section 2.1 of
// the host requirements. A prefix beginning with a digit is therefore a
// perfectly good host name and malformed here.
//
// The divergence runs the other way too, which is why neither rule contains
// the other. A host name caps its total length and holds an
// internationalized label to decoding as Punycode and round tripping, and
// neither of those constrains a prefix. Reverse domain notation is only what
// "Implementations SHOULD use reverse DNS notation", a recommendation about
// how to choose a prefix rather than a requirement that it resolve.
//
// The name after the slash is looser than the prefix and unrelated to host
// names: it "MUST begin and end with an alphanumeric character" and may carry
// hyphens, underscores and dots in between. It may also be absent entirely,
// which is what an extension key is.
//
// https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
// https://www.rfc-editor.org/rfc/rfc1123
inline auto metadata_key(const JSON::StringView key, const bool prefix_required)
    -> bool {
  const auto slash{key.find('/')};
  JSON::StringView name{key};
  if (slash == JSON::StringView::npos) {
    if (prefix_required) {
      return false;
    }
  } else {
    if (slash == 0 || key.find('/', slash + 1) != JSON::StringView::npos) {
      return false;
    }
    const auto prefix{key.substr(0, slash)};
    bool start = true;
    char previous = '.';
    for (const char character : prefix) {
      if (character == '.') {
        if (start || !sourcemeta::core::is_alphanum(previous)) {
          return false;
        }
        start = true;
      } else {
        if ((start && !sourcemeta::core::is_alpha(character)) ||
            (!sourcemeta::core::is_alphanum(character) && character != '-')) {
          return false;
        }
        start = false;
      }
      previous = character;
    }
    if (start || !sourcemeta::core::is_alphanum(previous)) {
      return false;
    }
    name = key.substr(slash + 1);
  }
  return name.empty() ||
         (sourcemeta::core::is_alphanum(name.front()) &&
          sourcemeta::core::is_alphanum(name.back()) &&
          std::all_of(name.begin(), name.end(), [](const char character) {
            return sourcemeta::core::is_alphanum(character) ||
                   character == '_' || character == '-' || character == '.';
          }));
}

inline auto sampling_block(const JSON &value, const bool received = false)
    -> bool {
  const auto *type{field(value, "type", sourcemeta::core::MCP_HASH_TYPE)};
  if ((type == nullptr) || !type->is_string() ||
      !sourcemeta::core::internal::valid_meta(MCPProtocolVersion::V_2026_07_28,
                                              value)) {
    return false;
  }
  const auto name{type->to_string()};
  if (name == "text" || name == "image" || name == "audio") {
    return sourcemeta::core::internal::valid_content(
        MCPProtocolVersion::V_2026_07_28, value, received);
  }
  if (name == "tool_use") {
    const auto *input{field(value, "input", sourcemeta::core::MCP_HASH_INPUT)};
    return string_field(value, "id", sourcemeta::core::MCP_HASH_ID, true) &&
           string_field(value, "name", sourcemeta::core::MCP_HASH_NAME, true) &&
           (input != nullptr) && input->is_object();
  }
  if (name == "tool_result") {
    const auto *content{
        field(value, "content", sourcemeta::core::MCP_HASH_CONTENT)};
    if (!string_field(value, "toolUseId",
                      sourcemeta::core::MCP_HASH_TOOL_USE_ID, true) ||
        (content == nullptr) || !content->is_array() ||
        !optional_fields(value,
                         {{"isError", sourcemeta::core::MCP_HASH_IS_ERROR}},
                         JSON::Type::Boolean)) {
      return false;
    }
    for (const auto &block : content->as_array()) {
      if (!sourcemeta::core::internal::valid_content(
              MCPProtocolVersion::V_2026_07_28, block, received)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

inline auto sampling_message_block(const JSON &block, const bool tools,
                                   const JSON::StringView role,
                                   const bool answering,
                                   std::set<JSON::StringView> &pending,
                                   std::set<JSON::StringView> &next) -> bool {
  if (!sampling_block(block)) {
    return false;
  }
  const auto type{
      block.at("type", sourcemeta::core::MCP_HASH_TYPE).to_string()};
  if (type == "tool_result") {
    return tools && answering && role == "user" &&
           pending.erase(
               block.at("toolUseId", sourcemeta::core::MCP_HASH_TOOL_USE_ID)
                   .to_string()) == 1;
  }
  if (answering) {
    return false;
  }
  if (type == "tool_use") {
    return tools && role == "assistant" &&
           next.insert(
                   block.at("id", sourcemeta::core::MCP_HASH_ID).to_string())
               .second;
  }
  return true;
}

inline auto
sampling_messages(const JSON &messages,
                  const sourcemeta::core::MCPClientCapabilities &capabilities)
    -> bool {
  if (!messages.is_array()) {
    return false;
  }
  std::set<JSON::StringView> pending;
  for (const auto &message : messages.as_array()) {
    const auto *role{field(message, "role", sourcemeta::core::MCP_HASH_ROLE)};
    const auto *content{
        field(message, "content", sourcemeta::core::MCP_HASH_CONTENT)};
    if ((role == nullptr) || !role->is_string() ||
        (role->to_string() != "user" && role->to_string() != "assistant") ||
        (content == nullptr) ||
        !sourcemeta::core::internal::valid_meta(
            MCPProtocolVersion::V_2026_07_28, message)) {
      return false;
    }
    // Tool calls must be resolved by the immediately following user message,
    // which may contain only the corresponding tool results.
    const bool answering{!pending.empty()};
    std::set<JSON::StringView> next;
    if (content->is_array()) {
      for (const auto &block : content->as_array()) {
        if (!sampling_message_block(block, capabilities.sampling_tools,
                                    role->to_string(), answering, pending,
                                    next)) {
          return false;
        }
      }
    } else if (!sampling_message_block(*content, capabilities.sampling_tools,
                                       role->to_string(), answering, pending,
                                       next)) {
      return false;
    }
    if (!pending.empty()) {
      return false;
    }
    pending = std::move(next);
  }
  return pending.empty();
}

inline auto sampling_options(const JSON &params) -> bool {
  if (!string_field(params, "systemPrompt",
                    sourcemeta::core::MCP_HASH_SYSTEM_PROMPT) ||
      !optional_fields(
          params,
          {{"metadata", sourcemeta::core::MCP_HASH_METADATA},
           {"modelPreferences", sourcemeta::core::MCP_HASH_MODEL_PREFERENCES},
           {"toolChoice", sourcemeta::core::MCP_HASH_TOOL_CHOICE}},
          JSON::Type::Object)) {
    return false;
  }
  if (const auto *temperature{
          field(params, "temperature", sourcemeta::core::MCP_HASH_TEMPERATURE)};
      (temperature != nullptr) && !temperature->is_number()) {
    return false;
  }
  if (const auto *stop{field(params, "stopSequences",
                             sourcemeta::core::MCP_HASH_STOP_SEQUENCES)};
      (stop != nullptr) && !strings(*stop)) {
    return false;
  }
  if (const auto *choice{
          field(params, "toolChoice", sourcemeta::core::MCP_HASH_TOOL_CHOICE)};
      choice) {
    const auto *mode{field(*choice, "mode", sourcemeta::core::MCP_HASH_MODE)};
    if ((mode != nullptr) &&
        (!mode->is_string() ||
         (mode->to_string() != "auto" && mode->to_string() != "none" &&
          mode->to_string() != "required"))) {
      return false;
    }
  }
  if (const auto *preferences{
          field(params, "modelPreferences",
                sourcemeta::core::MCP_HASH_MODEL_PREFERENCES)};
      preferences) {
    for (const auto &[name, hash] :
         {std::pair{JSON::StringView{"costPriority"},
                    sourcemeta::core::MCP_HASH_COST_PRIORITY},
          std::pair{JSON::StringView{"speedPriority"},
                    sourcemeta::core::MCP_HASH_SPEED_PRIORITY},
          std::pair{JSON::StringView{"intelligencePriority"},
                    sourcemeta::core::MCP_HASH_INTELLIGENCE_PRIORITY}}) {
      const auto *value{field(*preferences, name, hash)};
      if ((value != nullptr) && (!value->is_number() || value->as_real() < 0 ||
                                 value->as_real() > 1)) {
        return false;
      }
    }
    if (const auto *hints{
            field(*preferences, "hints", sourcemeta::core::MCP_HASH_HINTS)};
        hints) {
      if (!hints->is_array()) {
        return false;
      }
      for (const auto &hint : hints->as_array()) {
        if (!hint.is_object() ||
            !string_field(hint, "name", sourcemeta::core::MCP_HASH_NAME)) {
          return false;
        }
      }
    }
  }
  if (const auto *tools{
          field(params, "tools", sourcemeta::core::MCP_HASH_TOOLS)};
      tools) {
    if (!tools->is_array()) {
      return false;
    }
    for (const auto &tool : tools->as_array()) {
      if (!sourcemeta::core::internal::valid_tool(
              MCPProtocolVersion::V_2026_07_28, tool)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace sourcemeta::core::internal::validation

namespace sourcemeta::core::internal {

inline auto valid_log_level(const JSON::StringView value) noexcept -> bool {
  return mcp_resolve_log_level(value).has_value();
}

inline auto valid_meta(const JSON &value) -> bool {
  const auto *meta{
      validation::field(value, "_meta", sourcemeta::core::MCP_HASH_META)};
  return meta == nullptr || meta->is_object();
}

inline auto valid_meta(const MCPProtocolVersion version, const JSON &value)
    -> bool {
  const auto *meta{
      validation::field(value, "_meta", sourcemeta::core::MCP_HASH_META)};
  return meta == nullptr || (version == MCPProtocolVersion::V_2026_07_28
                                 ? valid_metadata_object(*meta, false)
                                 : meta->is_object());
}

inline auto valid_metadata_object(const JSON &meta, const bool received)
    -> bool {
  if (!meta.is_object()) {
    return false;
  }
  for (const auto &entry : meta.as_object()) {
    if (!validation::metadata_key(entry.first, false)) {
      return false;
    }
  }
  // MCP states the three reserved tracing values as a requirement on the
  // sender, so a value that does not follow its format makes the metadata
  // invalid. The trace context grammar constrains how that is enforced, in one
  // direction only: a tracestate that does not parse must not decide the
  // outcome for a traceparent that does, while the reverse carries no such
  // protection and a traceparent that does not parse ends the walk before the
  // tracestate is looked at.
  //
  // That protection is for a receiver reading what someone else sent, so it
  // applies to received metadata alone. A sender has no such excuse, since the
  // requirement is on whoever emits the value, and nothing this module builds
  // should carry a value it would refuse to accept. So on the receive path a
  // tracestate is held to its format only when it arrives without a
  // traceparent, where there is no parse to protect, which is what keeps a
  // value an intermediary injected from costing the trace it was meant to
  // carry. Everywhere else all three are held to their formats.
  //
  // The remedy itself is a choice rather than a rule: MCP prescribes none for
  // a receiver, and the processing model that describes discarding is a
  // section the grammar marks as non-normative.
  // https://www.w3.org/TR/trace-context/#tracestate-header
  // https://modelcontextprotocol.io/specification/2026-07-28/basic/index#_meta
  const auto *traceparent{
      meta.try_at("traceparent", sourcemeta::core::MCP_HASH_TRACEPARENT)};
  if (traceparent != nullptr &&
      (!traceparent->is_string() ||
       !http_is_traceparent(traceparent->to_string()))) {
    return false;
  }
  if (!received || traceparent == nullptr) {
    if (const auto *tracestate{
            meta.try_at("tracestate", sourcemeta::core::MCP_HASH_TRACESTATE)};
        tracestate != nullptr &&
        (!tracestate->is_string() ||
         !http_is_tracestate(tracestate->to_string()))) {
      return false;
    }
  }
  if (const auto *baggage{
          meta.try_at("baggage", sourcemeta::core::MCP_HASH_BAGGAGE)};
      baggage != nullptr &&
      (!baggage->is_string() || !http_is_baggage(baggage->to_string()))) {
    return false;
  }
  return true;
}

inline auto valid_implementation(const MCPProtocolVersion version,
                                 const JSON &value, const bool received)
    -> bool {
  if (!value.is_object() ||
      !validation::string_field(value, "name", sourcemeta::core::MCP_HASH_NAME,
                                true) ||
      !validation::string_field(value, "version",
                                sourcemeta::core::MCP_HASH_VERSION, true)) {
    return false;
  }
  if (mcp_supports_implementation_title(version) &&
      !validation::string_field(value, "title",
                                sourcemeta::core::MCP_HASH_TITLE)) {
    return false;
  }
  return !mcp_supports_implementation_description(version) ||
         (validation::string_field(value, "description",
                                   sourcemeta::core::MCP_HASH_DESCRIPTION) &&
          validation::string_field(value, "websiteUrl",
                                   sourcemeta::core::MCP_HASH_WEBSITE_URL) &&
          validation::icons(value, received));
}

inline auto make_implementation(const MCPProtocolVersion version,
                                const MCPImplementation &value) -> JSON {
  auto result{JSON::make_object()};
  result.assign_assume_new("name", JSON{value.name},
                           sourcemeta::core::MCP_HASH_NAME);
  result.assign_assume_new("version", JSON{value.version},
                           sourcemeta::core::MCP_HASH_VERSION);
  if (mcp_supports_implementation_title(version) && !value.title.empty()) {
    result.assign_assume_new("title", JSON{value.title},
                             sourcemeta::core::MCP_HASH_TITLE);
  }
  if (mcp_supports_implementation_description(version)) {
    if (!value.description.empty()) {
      result.assign_assume_new("description", JSON{value.description},
                               sourcemeta::core::MCP_HASH_DESCRIPTION);
    }
    if (!value.website_url.empty()) {
      result.assign_assume_new("websiteUrl", JSON{value.website_url},
                               sourcemeta::core::MCP_HASH_WEBSITE_URL);
    }
    if (value.icons != nullptr) {
      result.assign_assume_new("icons", JSON{*value.icons},
                               sourcemeta::core::MCP_HASH_ICONS);
    }
  }
  require(valid_implementation(version, result),
          "Invalid MCP implementation information");
  return result;
}

inline auto valid_extensions(const JSON &value) -> bool {
  if (!value.is_object()) {
    return false;
  }
  for (const auto &entry : value.as_object()) {
    if (!validation::metadata_key(entry.first, true) ||
        !entry.second.is_object()) {
      return false;
    }
  }
  return true;
}

inline auto valid_capabilities(const MCPProtocolVersion version,
                               const JSON &value, const bool client) -> bool {
  if (!value.is_object()) {
    return false;
  }
  if (const auto *experimental{validation::field(
          value, "experimental", sourcemeta::core::MCP_HASH_EXPERIMENTAL)};
      experimental) {
    if (!experimental->is_object()) {
      return false;
    }
    for (const auto &entry : experimental->as_object()) {
      if (!entry.second.is_object()) {
        return false;
      }
    }
  }
  if (version == MCPProtocolVersion::V_2026_07_28) {
    if (const auto *extensions{validation::field(
            value, "extensions", sourcemeta::core::MCP_HASH_EXTENSIONS)};
        (extensions != nullptr) && !valid_extensions(*extensions)) {
      return false;
    }
  }
  if (version == MCPProtocolVersion::V_2025_11_25) {
    if (const auto *tasks{validation::field(value, "tasks",
                                            sourcemeta::core::MCP_HASH_TASKS)};
        tasks) {
      if (!tasks->is_object() ||
          !validation::optional_fields(
              *tasks,
              {{"list", sourcemeta::core::MCP_HASH_LIST},
               {"cancel", sourcemeta::core::MCP_HASH_CANCEL},
               {"requests", sourcemeta::core::MCP_HASH_REQUESTS}},
              JSON::Type::Object)) {
        return false;
      }
      if (const auto *requests{validation::field(
              *tasks, "requests", sourcemeta::core::MCP_HASH_REQUESTS)};
          requests) {
        if (!validation::optional_fields(
                *requests,
                {{"sampling", sourcemeta::core::MCP_HASH_SAMPLING},
                 {"elicitation", sourcemeta::core::MCP_HASH_ELICITATION},
                 {"tools", sourcemeta::core::MCP_HASH_TOOLS}},
                JSON::Type::Object)) {
          return false;
        }
        for (const auto &[name, hash] :
             {std::pair{JSON::StringView{"sampling"},
                        sourcemeta::core::MCP_HASH_SAMPLING},
              std::pair{JSON::StringView{"elicitation"},
                        sourcemeta::core::MCP_HASH_ELICITATION},
              std::pair{JSON::StringView{"tools"},
                        sourcemeta::core::MCP_HASH_TOOLS}}) {
          if (const auto *group{validation::field(*requests, name, hash)};
              (group != nullptr) &&
              !validation::optional_fields(
                  *group,
                  {{"createMessage", sourcemeta::core::MCP_HASH_CREATE_MESSAGE},
                   {"create", sourcemeta::core::MCP_HASH_CREATE},
                   {"call", sourcemeta::core::MCP_HASH_CALL}},
                  JSON::Type::Object)) {
            return false;
          }
        }
      }
    }
  }
  if (client) {
    if (!validation::optional_fields(
            value,
            {{"roots", sourcemeta::core::MCP_HASH_ROOTS},
             {"sampling", sourcemeta::core::MCP_HASH_SAMPLING}},
            JSON::Type::Object)) {
      return false;
    }
    if (const auto *roots{validation::field(value, "roots",
                                            sourcemeta::core::MCP_HASH_ROOTS)};
        (roots != nullptr) && version != MCPProtocolVersion::V_2026_07_28 &&
        !validation::optional_fields(
            *roots, {{"listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED}},
            JSON::Type::Boolean)) {
      return false;
    }
    if (const auto *sampling{validation::field(
            value, "sampling", sourcemeta::core::MCP_HASH_SAMPLING)};
        (sampling != nullptr) &&
        mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25) &&
        !validation::optional_fields(
            *sampling,
            {{"tools", sourcemeta::core::MCP_HASH_TOOLS},
             {"context", sourcemeta::core::MCP_HASH_CONTEXT}},
            JSON::Type::Object)) {
      return false;
    }
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_06_18)) {
      if (!validation::optional_fields(
              value, {{"elicitation", sourcemeta::core::MCP_HASH_ELICITATION}},
              JSON::Type::Object)) {
        return false;
      }
      if (const auto *elicitation{validation::field(
              value, "elicitation", sourcemeta::core::MCP_HASH_ELICITATION)};
          (elicitation != nullptr) &&
          mcp_protocol_version_at_least(version,
                                        MCPProtocolVersion::V_2025_11_25) &&
          !validation::optional_fields(
              *elicitation,
              {{"form", sourcemeta::core::MCP_HASH_FORM},
               {"url", sourcemeta::core::MCP_HASH_URL}},
              JSON::Type::Object)) {
        return false;
      }
    }
  } else {
    if (!validation::optional_fields(
            value,
            {{"logging", sourcemeta::core::MCP_HASH_LOGGING},
             {"completions", sourcemeta::core::MCP_HASH_COMPLETIONS},
             {"prompts", sourcemeta::core::MCP_HASH_PROMPTS},
             {"resources", sourcemeta::core::MCP_HASH_RESOURCES},
             {"tools", sourcemeta::core::MCP_HASH_TOOLS}},
            JSON::Type::Object)) {
      return false;
    }
    for (const auto &[name, hash] :
         {std::pair{JSON::StringView{"prompts"},
                    sourcemeta::core::MCP_HASH_PROMPTS},
          std::pair{JSON::StringView{"resources"},
                    sourcemeta::core::MCP_HASH_RESOURCES},
          std::pair{JSON::StringView{"tools"},
                    sourcemeta::core::MCP_HASH_TOOLS}}) {
      if (const auto *item{validation::field(value, name, hash)};
          (item != nullptr) &&
          !validation::optional_fields(
              *item, {{"listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED}},
              JSON::Type::Boolean)) {
        return false;
      }
    }
    if (const auto *resources{validation::field(
            value, "resources", sourcemeta::core::MCP_HASH_RESOURCES)};
        (resources != nullptr) &&
        !validation::optional_fields(
            *resources, {{"subscribe", sourcemeta::core::MCP_HASH_SUBSCRIBE}},
            JSON::Type::Boolean)) {
      return false;
    }
  }
  return true;
}

inline auto valid_tool(const MCPProtocolVersion version, const JSON &value)
    -> bool {
  if (!validation::base(value) || !valid_meta(version, value)) {
    return false;
  }
  const auto *input{validation::field(value, "inputSchema",
                                      sourcemeta::core::MCP_HASH_INPUT_SCHEMA)};
  if ((input == nullptr) || !validation::object_schema(*input)) {
    return false;
  }
  if (mcp_supports_output_schema(version)) {
    if (const auto *output{validation::field(
            value, "outputSchema", sourcemeta::core::MCP_HASH_OUTPUT_SCHEMA)};
        (output != nullptr) &&
        (!output->is_object() || (version != MCPProtocolVersion::V_2026_07_28 &&
                                  !validation::object_schema(*output)))) {
      return false;
    }
  }
  if (const auto *item{validation::field(
          value, "annotations", sourcemeta::core::MCP_HASH_ANNOTATIONS)};
      (item != nullptr) &&
      (!item->is_object() ||
       !validation::string_field(*item, "title",
                                 sourcemeta::core::MCP_HASH_TITLE) ||
       !validation::optional_fields(
           *item,
           {{"readOnlyHint", sourcemeta::core::MCP_HASH_READ_ONLY_HINT},
            {"destructiveHint", sourcemeta::core::MCP_HASH_DESTRUCTIVE_HINT},
            {"idempotentHint", sourcemeta::core::MCP_HASH_IDEMPOTENT_HINT},
            {"openWorldHint", sourcemeta::core::MCP_HASH_OPEN_WORLD_HINT}},
           JSON::Type::Boolean))) {
    return false;
  }
  return true;
}

inline auto valid_resource(const MCPProtocolVersion version, const JSON &value,
                           const bool contents, const bool is_template,
                           const bool received) -> bool {
  if (!value.is_object() || !valid_meta(version, value) ||
      (is_template || received
           ? !validation::string_field(
                 value, is_template ? "uriTemplate" : "uri",
                 is_template ? sourcemeta::core::MCP_HASH_URI_TEMPLATE
                             : sourcemeta::core::MCP_HASH_URI,
                 true)
           : !validation::uri_field(value, "uri",
                                    sourcemeta::core::MCP_HASH_URI, true)) ||
      !(received
            ? validation::string_field(value, "mimeType",
                                       sourcemeta::core::MCP_HASH_MIME_TYPE)
            : validation::media_type_field(
                  value, "mimeType", sourcemeta::core::MCP_HASH_MIME_TYPE))) {
    return false;
  }
  if (contents) {
    const auto *text{
        validation::field(value, "text", sourcemeta::core::MCP_HASH_TEXT)};
    const auto *blob{
        validation::field(value, "blob", sourcemeta::core::MCP_HASH_BLOB)};
    return ((text != nullptr) && text->is_string()) ||
           ((blob != nullptr) && blob->is_string());
  }
  const auto *size{
      validation::field(value, "size", sourcemeta::core::MCP_HASH_SIZE)};
  return validation::base(value, received) &&
         validation::annotations(value, received) &&
         ((size == nullptr) || (size->is_integral() && *size >= JSON{0}));
}

inline auto valid_content(const MCPProtocolVersion version, const JSON &value,
                          const bool received) -> bool {
  if (!value.is_object() || !valid_meta(version, value) ||
      !validation::annotations(value, received)) {
    return false;
  }
  const auto *type{
      validation::field(value, "type", sourcemeta::core::MCP_HASH_TYPE)};
  if ((type == nullptr) || !type->is_string()) {
    return false;
  }
  const auto name{type->to_string()};
  if (name == "text") {
    return validation::string_field(value, "text",
                                    sourcemeta::core::MCP_HASH_TEXT, true);
  }
  if (name == "image" || name == "audio") {
    return validation::string_field(value, "data",
                                    sourcemeta::core::MCP_HASH_DATA, true) &&
           (received ? validation::string_field(
                           value, "mimeType",
                           sourcemeta::core::MCP_HASH_MIME_TYPE, true)
                     : validation::media_type_field(
                           value, "mimeType",
                           sourcemeta::core::MCP_HASH_MIME_TYPE, true));
  }
  if (name == "resource") {
    const auto *resource{validation::field(
        value, "resource", sourcemeta::core::MCP_HASH_RESOURCE)};
    return (resource != nullptr) &&
           valid_resource(version, *resource, true, false, received);
  }
  return name == "resource_link" &&
         mcp_supports_resource_link_content(version) &&
         valid_resource(version, value, false, false, received);
}

inline auto valid_prompt(const MCPProtocolVersion version, const JSON &value)
    -> bool {
  if (!validation::base(value) || !valid_meta(version, value)) {
    return false;
  }
  const auto *arguments{validation::field(
      value, "arguments", sourcemeta::core::MCP_HASH_ARGUMENTS)};
  if (arguments == nullptr) {
    return true;
  }
  if (!arguments->is_array()) {
    return false;
  }
  for (const auto &argument : arguments->as_array()) {
    if (!validation::base(argument) || !valid_meta(version, argument) ||
        !validation::optional_fields(
            argument, {{"required", sourcemeta::core::MCP_HASH_REQUIRED}},
            JSON::Type::Boolean)) {
      return false;
    }
  }
  return true;
}

inline auto valid_input_requests(const JSON &value,
                                 const MCPClientCapabilities &capabilities)
    -> bool {
  if (!value.is_object()) {
    return false;
  }
  for (const auto &entry : value.as_object()) {
    const auto &request{entry.second};
    const auto *method{validation::field(request, "method",
                                         sourcemeta::core::MCP_HASH_METHOD)};
    const auto *params{validation::field(request, "params",
                                         sourcemeta::core::MCP_HASH_PARAMS)};
    if ((method == nullptr) || !method->is_string() ||
        request.defines("id", sourcemeta::core::MCP_HASH_ID) ||
        request.defines("jsonrpc", sourcemeta::core::MCP_HASH_JSONRPC)) {
      return false;
    }
    // Nested server requests do not carry the outer stateless client metadata,
    // but any optional metadata they do provide must still be well formed.
    if (params != nullptr &&
        (!params->is_object() ||
         !valid_meta(MCPProtocolVersion::V_2026_07_28, *params))) {
      return false;
    }
    const auto name{method->to_string()};
    if (name == MCP_METHOD_ROOTS_LIST) {
      if (!capabilities.roots) {
        return false;
      }
    } else if (name == MCP_METHOD_ELICITATION_CREATE) {
      if ((params == nullptr) || !params->is_object() ||
          !validation::string_field(*params, "message",
                                    sourcemeta::core::MCP_HASH_MESSAGE, true)) {
        return false;
      }
      const auto *mode{
          validation::field(*params, "mode", sourcemeta::core::MCP_HASH_MODE)};
      if ((mode != nullptr) &&
          (!mode->is_string() ||
           (mode->to_string() != "form" && mode->to_string() != "url"))) {
        return false;
      }
      if ((mode != nullptr) && mode->to_string() == "url") {
        const auto *url{
            validation::field(*params, "url", sourcemeta::core::MCP_HASH_URL)};
        if (!capabilities.elicitation_url || (url == nullptr) ||
            !url->is_string()) {
          return false;
        }
        // The user is sent out of band to this address, which the
        // specification requires to be a URL rather than a reference to
        // resolve against something
        // https://modelcontextprotocol.io/specification/2026-07-28/client/elicitation#url-mode-elicitation-requests
        if (!sourcemeta::core::URI::is_uri(url->to_string())) {
          return false;
        }
      } else {
        const auto *schema{
            validation::field(*params, "requestedSchema",
                              sourcemeta::core::MCP_HASH_REQUESTED_SCHEMA)};
        const bool implicit_form{capabilities.elicitation &&
                                 !capabilities.elicitation_url};
        if ((!capabilities.elicitation_form && !implicit_form) ||
            (schema == nullptr) || !validation::object_schema(*schema)) {
          return false;
        }
        // A form names the fields it asks for, so the property map is
        // mandatory. What each property contains is a schema, which this
        // module deliberately does not interpret
        // https://modelcontextprotocol.io/specification/2026-07-28/schema#elicitrequestformparams
        const auto *properties{validation::field(
            *schema, "properties", sourcemeta::core::MCP_HASH_PROPERTIES)};
        if ((properties == nullptr) || !properties->is_object()) {
          return false;
        }
      }
    } else if (name == MCP_METHOD_SAMPLING_CREATE_MESSAGE) {
      if (!capabilities.sampling || (params == nullptr) ||
          !params->is_object() || !validation::sampling_options(*params)) {
        return false;
      }
      if (const auto *context{
              validation::field(*params, "includeContext",
                                sourcemeta::core::MCP_HASH_INCLUDE_CONTEXT)};
          context) {
        if (!context->is_string() ||
            (context->to_string() != "none" &&
             context->to_string() != "thisServer" &&
             context->to_string() != "allServers") ||
            (context->to_string() != "none" &&
             !capabilities.sampling_context)) {
          return false;
        }
      }
      const auto *messages{validation::field(
          *params, "messages", sourcemeta::core::MCP_HASH_MESSAGES)};
      const auto *tokens{validation::field(
          *params, "maxTokens", sourcemeta::core::MCP_HASH_MAX_TOKENS)};
      if ((messages == nullptr) ||
          !validation::sampling_messages(*messages, capabilities) ||
          (tokens == nullptr) || !tokens->is_integral()) {
        return false;
      }
      if (((validation::field(*params, "tools",
                              sourcemeta::core::MCP_HASH_TOOLS) != nullptr) ||
           (validation::field(*params, "toolChoice",
                              sourcemeta::core::MCP_HASH_TOOL_CHOICE) !=
            nullptr)) &&
          !capabilities.sampling_tools) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

inline auto valid_input_response(const JSON::StringView method,
                                 const JSON &request, const JSON &value)
    -> bool {
  if (!value.is_object() ||
      !valid_meta(MCPProtocolVersion::V_2026_07_28, value)) {
    return false;
  }
  if (method == MCP_METHOD_ROOTS_LIST) {
    const auto *roots{
        validation::field(value, "roots", sourcemeta::core::MCP_HASH_ROOTS)};
    if ((roots == nullptr) || !roots->is_array()) {
      return false;
    }
    for (const auto &root : roots->as_array()) {
      const auto *uri{
          validation::field(root, "uri", sourcemeta::core::MCP_HASH_URI)};
      if ((uri == nullptr) || !uri->is_string() ||
          !uri->to_string().starts_with("file://") ||
          !validation::string_field(root, "name",
                                    sourcemeta::core::MCP_HASH_NAME) ||
          !valid_meta(MCPProtocolVersion::V_2026_07_28, root)) {
        return false;
      }
    }
    return true;
  }
  if (method == MCP_METHOD_ELICITATION_CREATE) {
    const auto *action{
        validation::field(value, "action", sourcemeta::core::MCP_HASH_ACTION)};
    if ((action == nullptr) || !action->is_string() ||
        (action->to_string() != "accept" && action->to_string() != "decline" &&
         action->to_string() != "cancel")) {
      return false;
    }
    if (const auto *content{validation::field(
            value, "content", sourcemeta::core::MCP_HASH_CONTENT)};
        content) {
      const auto *params{validation::field(request, "params",
                                           sourcemeta::core::MCP_HASH_PARAMS)};
      const auto *mode{params != nullptr
                           ? validation::field(*params, "mode",
                                               sourcemeta::core::MCP_HASH_MODE)
                           : nullptr};
      if (action->to_string() != "accept" ||
          (mode != nullptr &&
           (!mode->is_string() || mode->to_string() != "form")) ||
          !content->is_object()) {
        return false;
      }
      for (const auto &entry : content->as_object()) {
        if (entry.second.is_real() && !std::isfinite(entry.second.to_real())) {
          return false;
        }
        if (!entry.second.is_string() && !entry.second.is_number() &&
            !entry.second.is_boolean() && !validation::strings(entry.second)) {
          return false;
        }
      }
    }
    return true;
  }
  if (method == MCP_METHOD_SAMPLING_CREATE_MESSAGE) {
    const auto *role{
        validation::field(value, "role", sourcemeta::core::MCP_HASH_ROLE)};
    const auto *content{validation::field(value, "content",
                                          sourcemeta::core::MCP_HASH_CONTENT)};
    if ((role == nullptr) || !role->is_string() ||
        (role->to_string() != "user" && role->to_string() != "assistant") ||
        (content == nullptr) ||
        !validation::string_field(value, "model",
                                  sourcemeta::core::MCP_HASH_MODEL, true) ||
        !validation::string_field(value, "stopReason",
                                  sourcemeta::core::MCP_HASH_STOP_REASON)) {
      return false;
    }
    if (!content->is_array()) {
      return validation::sampling_block(*content, true);
    }
    return std::all_of(content->as_array().begin(), content->as_array().end(),
                       [](const auto &block) {
                         return validation::sampling_block(block, true);
                       });
  }
  return false;
}

} // namespace sourcemeta::core::internal

#endif
