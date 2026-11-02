#include <sourcemeta/core/mcp.h>

#include "helpers.h"
#include "validation.h"

#include <sourcemeta/core/json.h>

#include <utility> // std::move, std::pair

namespace {
template <typename Capabilities>
auto capability_settings(const Capabilities &capabilities,
                         const sourcemeta::core::JSON::StringView name,
                         const sourcemeta::core::JSON::Object::hash_type hash)
    -> sourcemeta::core::JSON {
  const auto *value{capabilities.source && capabilities.source->is_object()
                        ? capabilities.source->try_at(name, hash)
                        : nullptr};
  return value && value->is_object() ? *value
                                     : sourcemeta::core::JSON::make_object();
}
auto capability_flag(
    const sourcemeta::core::JSON &capabilities,
    const sourcemeta::core::JSON::StringView parent,
    const sourcemeta::core::JSON::Object::hash_type parent_hash,
    const sourcemeta::core::JSON::StringView name,
    const sourcemeta::core::JSON::Object::hash_type name_hash) -> bool {
  const auto *settings{capabilities.try_at(parent, parent_hash)};
  const auto *value{settings != nullptr ? settings->try_at(name, name_hash)
                                        : nullptr};
  return value != nullptr && value->is_boolean() && value->to_boolean();
}

auto emit_capability(
    sourcemeta::core::JSON &result,
    const sourcemeta::core::MCPServerCapabilities &capabilities,
    const sourcemeta::core::JSON::StringView name,
    const sourcemeta::core::JSON::Object::hash_type hash, const bool enabled,
    const bool changed, const bool subscribe) -> void {
  using namespace sourcemeta::core;
  if (!enabled && !changed && !subscribe) {
    return;
  }
  auto settings{capability_settings(capabilities, name, hash)};
  if (changed ||
      !settings.defines("listChanged",
                        sourcemeta::core::MCP_HASH_LIST_CHANGED) ||
      settings.at("listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED) !=
          JSON{false}) {
    settings.erase("listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED);
  }
  if (changed) {
    settings.assign_assume_new("listChanged", JSON{true},
                               sourcemeta::core::MCP_HASH_LIST_CHANGED);
  }
  if (name == "resources") {
    if (subscribe ||
        !settings.defines("subscribe", sourcemeta::core::MCP_HASH_SUBSCRIBE) ||
        settings.at("subscribe", sourcemeta::core::MCP_HASH_SUBSCRIBE) !=
            JSON{false}) {
      settings.erase("subscribe", sourcemeta::core::MCP_HASH_SUBSCRIBE);
    }
    if (subscribe) {
      settings.assign_assume_new("subscribe", JSON{true},
                                 sourcemeta::core::MCP_HASH_SUBSCRIBE);
    }
  }
  result.assign_assume_new(JSON::String{name}, std::move(settings), hash);
}
} // namespace

namespace sourcemeta::core {

auto mcp_parse_client_capabilities(const MCPProtocolVersion version,
                                   const sourcemeta::core::JSON &capabilities)
    -> MCPClientCapabilities {
  MCPClientCapabilities result;
  result.source = capabilities;
  internal::require(internal::valid_capabilities(version, capabilities, true),
                    "Invalid client capabilities");

  const auto *roots_field{capabilities.try_at("roots", MCP_HASH_ROOTS)};
  if (roots_field != nullptr && roots_field->is_object()) {
    result.roots = true;
    const auto *list_changed_field{
        roots_field->try_at("listChanged", MCP_HASH_LIST_CHANGED)};
    if (version != MCPProtocolVersion::V_2026_07_28 &&
        list_changed_field != nullptr && list_changed_field->is_boolean()) {
      result.roots_list_changed = list_changed_field->to_boolean();
    }
  }

  const auto *sampling_field{
      capabilities.try_at("sampling", MCP_HASH_SAMPLING)};
  if (sampling_field != nullptr && sampling_field->is_object()) {
    result.sampling = true;
    const auto *context_field{
        sampling_field->try_at("context", MCP_HASH_CONTEXT)};
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25) &&
        context_field != nullptr && context_field->is_object()) {
      result.sampling_context = true;
    }
    const auto *tools_field{sampling_field->try_at("tools", MCP_HASH_TOOLS)};
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25) &&
        tools_field != nullptr && tools_field->is_object()) {
      result.sampling_tools = true;
    }
  }

  const auto *elicitation_field{
      capabilities.try_at("elicitation", MCP_HASH_ELICITATION)};
  if (mcp_protocol_version_at_least(version,
                                    MCPProtocolVersion::V_2025_06_18) &&
      elicitation_field != nullptr && elicitation_field->is_object()) {
    result.elicitation = true;
    const auto *form_field{elicitation_field->try_at("form", MCP_HASH_FORM)};
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25) &&
        form_field != nullptr && form_field->is_object()) {
      result.elicitation_form = true;
    }
    const auto *url_field{elicitation_field->try_at("url", MCP_HASH_URL)};
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25) &&
        url_field != nullptr && url_field->is_object()) {
      result.elicitation_url = true;
    }
  }

  const auto *extensions_field{
      capabilities.try_at("extensions", MCP_HASH_EXTENSIONS)};
  if (version == MCPProtocolVersion::V_2026_07_28 &&
      extensions_field != nullptr && extensions_field->is_object()) {
    result.extensions = *extensions_field;
  }

  const auto *experimental_field{
      capabilities.try_at("experimental", MCP_HASH_EXPERIMENTAL)};
  if (experimental_field != nullptr && experimental_field->is_object()) {
    result.experimental = *experimental_field;
  }

  return result;
}

auto mcp_serialize_client_capabilities(
    const MCPProtocolVersion version, const MCPClientCapabilities &capabilities)
    -> sourcemeta::core::JSON {
  internal::require(!capabilities.experimental ||
                        capabilities.experimental->is_object(),
                    "Invalid experimental capabilities");
  if (version == MCPProtocolVersion::V_2026_07_28 && capabilities.extensions) {
    internal::require(internal::valid_extensions(*capabilities.extensions),
                      "Invalid extension capabilities");
  }
  internal::require(!capabilities.source || capabilities.source->is_object(),
                    "Invalid capability source");
  auto result{capabilities.source.value_or(JSON::make_object())};
  // Preserve opaque settings while removing features absent from this revision.
  if (version != MCPProtocolVersion::V_2025_11_25) {
    result.erase("tasks", sourcemeta::core::MCP_HASH_TASKS);
  }
  result.erase("roots", sourcemeta::core::MCP_HASH_ROOTS);
  result.erase("sampling", sourcemeta::core::MCP_HASH_SAMPLING);
  result.erase("elicitation", sourcemeta::core::MCP_HASH_ELICITATION);
  result.erase("extensions", sourcemeta::core::MCP_HASH_EXTENSIONS);
  result.erase("experimental", sourcemeta::core::MCP_HASH_EXPERIMENTAL);

  if (capabilities.roots || (capabilities.roots_list_changed &&
                             version != MCPProtocolVersion::V_2026_07_28)) {
    auto roots_object{capability_settings(capabilities, "roots",
                                          sourcemeta::core::MCP_HASH_ROOTS)};
    if (version == MCPProtocolVersion::V_2026_07_28 ||
        (!capabilities.roots_list_changed &&
         (!roots_object.defines("listChanged",
                                sourcemeta::core::MCP_HASH_LIST_CHANGED) ||
          roots_object.at("listChanged",
                          sourcemeta::core::MCP_HASH_LIST_CHANGED) !=
              JSON{false}))) {
      roots_object.erase("listChanged",
                         sourcemeta::core::MCP_HASH_LIST_CHANGED);
    }
    if (capabilities.roots_list_changed &&
        version != MCPProtocolVersion::V_2026_07_28) {
      // The snapshot may already carry this setting, which the erase above
      // deliberately left in place
      roots_object.assign("listChanged", sourcemeta::core::JSON{true});
    }
    result.assign_assume_new("roots", std::move(roots_object),
                             sourcemeta::core::MCP_HASH_ROOTS);
  }

  if (capabilities.sampling || capabilities.sampling_context ||
      capabilities.sampling_tools) {
    auto sampling_object{capability_settings(
        capabilities, "sampling", sourcemeta::core::MCP_HASH_SAMPLING)};
    if (!mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_11_25)) {
      sampling_object.erase("context", sourcemeta::core::MCP_HASH_CONTEXT);
      sampling_object.erase("tools", sourcemeta::core::MCP_HASH_TOOLS);
    }
    if (!capabilities.sampling_context) {
      sampling_object.erase("context", sourcemeta::core::MCP_HASH_CONTEXT);
    }
    if (!capabilities.sampling_tools) {
      sampling_object.erase("tools", sourcemeta::core::MCP_HASH_TOOLS);
    }
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25)) {
      if (capabilities.sampling_context) {
        if (!sampling_object.defines("context",
                                     sourcemeta::core::MCP_HASH_CONTEXT)) {
          sampling_object.assign_assume_new("context", JSON::make_object(),
                                            sourcemeta::core::MCP_HASH_CONTEXT);
        }
      }
      if (capabilities.sampling_tools) {
        if (!sampling_object.defines("tools",
                                     sourcemeta::core::MCP_HASH_TOOLS)) {
          sampling_object.assign_assume_new("tools", JSON::make_object(),
                                            sourcemeta::core::MCP_HASH_TOOLS);
        }
      }
    }
    result.assign_assume_new("sampling", std::move(sampling_object),
                             sourcemeta::core::MCP_HASH_SAMPLING);
  }

  if (mcp_protocol_version_at_least(version,
                                    MCPProtocolVersion::V_2025_06_18) &&
      (capabilities.elicitation || capabilities.elicitation_form ||
       capabilities.elicitation_url)) {
    auto elicitation_object{capability_settings(
        capabilities, "elicitation", sourcemeta::core::MCP_HASH_ELICITATION)};
    if (!mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_11_25) ||
        !capabilities.elicitation_form) {
      elicitation_object.erase("form", sourcemeta::core::MCP_HASH_FORM);
    }
    if (!mcp_protocol_version_at_least(version,
                                       MCPProtocolVersion::V_2025_11_25) ||
        !capabilities.elicitation_url) {
      elicitation_object.erase("url", sourcemeta::core::MCP_HASH_URL);
    }
    if (mcp_protocol_version_at_least(version,
                                      MCPProtocolVersion::V_2025_11_25)) {
      if (capabilities.elicitation_form) {
        if (!elicitation_object.defines("form",
                                        sourcemeta::core::MCP_HASH_FORM)) {
          elicitation_object.assign_assume_new("form", JSON::make_object(),
                                               sourcemeta::core::MCP_HASH_FORM);
        }
      }
      if (capabilities.elicitation_url) {
        if (!elicitation_object.defines("url",
                                        sourcemeta::core::MCP_HASH_URL)) {
          elicitation_object.assign_assume_new("url", JSON::make_object(),
                                               sourcemeta::core::MCP_HASH_URL);
        }
      }
    }
    result.assign_assume_new("elicitation", std::move(elicitation_object),
                             sourcemeta::core::MCP_HASH_ELICITATION);
  }

  if (version == MCPProtocolVersion::V_2026_07_28 &&
      capabilities.extensions.has_value() &&
      capabilities.extensions->is_object()) {
    result.assign_assume_new(
        "extensions", sourcemeta::core::JSON{capabilities.extensions.value()},
        sourcemeta::core::MCP_HASH_EXTENSIONS);
  }

  if (capabilities.experimental.has_value() &&
      capabilities.experimental->is_object()) {
    result.assign_assume_new(
        "experimental",
        sourcemeta::core::JSON{capabilities.experimental.value()},
        sourcemeta::core::MCP_HASH_EXPERIMENTAL);
  }

  internal::require(internal::valid_capabilities(version, result, true),
                    "Invalid capability map");
  return result;
}

auto mcp_parse_server_capabilities(const MCPProtocolVersion version,
                                   const JSON &capabilities)
    -> MCPServerCapabilities {
  internal::require(internal::valid_capabilities(version, capabilities, false),
                    "Invalid server capabilities");
  MCPServerCapabilities result;
  result.source = capabilities;
  result.prompts =
      capabilities.defines("prompts", sourcemeta::core::MCP_HASH_PROMPTS);
  result.resources =
      capabilities.defines("resources", sourcemeta::core::MCP_HASH_RESOURCES);
  result.tools =
      capabilities.defines("tools", sourcemeta::core::MCP_HASH_TOOLS);
  result.logging =
      capabilities.defines("logging", sourcemeta::core::MCP_HASH_LOGGING);
  result.completions = capabilities.defines(
      "completions", sourcemeta::core::MCP_HASH_COMPLETIONS);
  result.prompts_list_changed = capability_flag(
      capabilities, "prompts", sourcemeta::core::MCP_HASH_PROMPTS,
      "listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED);
  result.resources_list_changed = capability_flag(
      capabilities, "resources", sourcemeta::core::MCP_HASH_RESOURCES,
      "listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED);
  result.resources_subscribe = capability_flag(
      capabilities, "resources", sourcemeta::core::MCP_HASH_RESOURCES,
      "subscribe", sourcemeta::core::MCP_HASH_SUBSCRIBE);
  result.tools_list_changed =
      capability_flag(capabilities, "tools", sourcemeta::core::MCP_HASH_TOOLS,
                      "listChanged", sourcemeta::core::MCP_HASH_LIST_CHANGED);
  if (version == MCPProtocolVersion::V_2026_07_28 &&
      capabilities.defines("extensions",
                           sourcemeta::core::MCP_HASH_EXTENSIONS)) {
    result.extensions =
        capabilities.at("extensions", sourcemeta::core::MCP_HASH_EXTENSIONS);
  }
  if (capabilities.defines("experimental",
                           sourcemeta::core::MCP_HASH_EXPERIMENTAL)) {
    result.experimental = capabilities.at(
        "experimental", sourcemeta::core::MCP_HASH_EXPERIMENTAL);
  }
  return result;
}

auto mcp_serialize_server_capabilities(
    const MCPProtocolVersion version, const MCPServerCapabilities &capabilities)
    -> JSON {
  internal::require(!capabilities.source || capabilities.source->is_object(),
                    "Invalid capability source");
  auto result{capabilities.source.value_or(JSON::make_object())};
  for (const auto &[name, hash] :
       {std::pair{JSON::StringView{"prompts"},
                  sourcemeta::core::MCP_HASH_PROMPTS},
        std::pair{JSON::StringView{"resources"},
                  sourcemeta::core::MCP_HASH_RESOURCES},
        std::pair{JSON::StringView{"tools"}, sourcemeta::core::MCP_HASH_TOOLS},
        std::pair{JSON::StringView{"logging"},
                  sourcemeta::core::MCP_HASH_LOGGING},
        std::pair{JSON::StringView{"completions"},
                  sourcemeta::core::MCP_HASH_COMPLETIONS},
        std::pair{JSON::StringView{"extensions"},
                  sourcemeta::core::MCP_HASH_EXTENSIONS},
        std::pair{JSON::StringView{"experimental"},
                  sourcemeta::core::MCP_HASH_EXPERIMENTAL}}) {
    result.erase(name, hash);
  }
  if (version != MCPProtocolVersion::V_2025_11_25) {
    result.erase("tasks", sourcemeta::core::MCP_HASH_TASKS);
  }
  emit_capability(result, capabilities, "prompts",
                  sourcemeta::core::MCP_HASH_PROMPTS, capabilities.prompts,
                  capabilities.prompts_list_changed, false);
  emit_capability(result, capabilities, "resources",
                  sourcemeta::core::MCP_HASH_RESOURCES, capabilities.resources,
                  capabilities.resources_list_changed,
                  capabilities.resources_subscribe);
  emit_capability(result, capabilities, "tools",
                  sourcemeta::core::MCP_HASH_TOOLS, capabilities.tools,
                  capabilities.tools_list_changed, false);
  if (capabilities.logging) {
    result.assign_assume_new(
        "logging",
        capability_settings(capabilities, "logging",
                            sourcemeta::core::MCP_HASH_LOGGING),
        sourcemeta::core::MCP_HASH_LOGGING);
  }
  if (capabilities.completions) {
    result.assign_assume_new(
        "completions",
        capability_settings(capabilities, "completions",
                            sourcemeta::core::MCP_HASH_COMPLETIONS),
        sourcemeta::core::MCP_HASH_COMPLETIONS);
  }
  if (version == MCPProtocolVersion::V_2026_07_28 && capabilities.extensions) {
    result.assign_assume_new("extensions",
                             sourcemeta::core::JSON{*capabilities.extensions},
                             sourcemeta::core::MCP_HASH_EXTENSIONS);
  }
  if (capabilities.experimental) {
    result.assign_assume_new("experimental",
                             sourcemeta::core::JSON{*capabilities.experimental},
                             sourcemeta::core::MCP_HASH_EXPERIMENTAL);
  }
  internal::require(internal::valid_capabilities(version, result, false),
                    "Invalid capability map");
  return result;
}

} // namespace sourcemeta::core
