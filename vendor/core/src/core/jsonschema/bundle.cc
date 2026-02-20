#include <sourcemeta/core/jsonschema.h>

#include "helpers.h"

#include <cassert>       // assert
#include <functional>    // std::reference_wrapper
#include <sstream>       // std::ostringstream
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace {

auto is_official_metaschema_reference(
    const sourcemeta::core::WeakPointer &pointer,
    const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  return pointer.back().to_property() == "$schema" &&
         sourcemeta::core::schema_resolver(destination).has_value();
}

auto dependencies_internal(const sourcemeta::core::JSON &schema,
                           const sourcemeta::core::SchemaWalker &walker,
                           const sourcemeta::core::SchemaResolver &resolver,
                           const sourcemeta::core::DependencyCallback &callback,
                           std::string_view default_dialect,
                           std::string_view default_id,
                           const sourcemeta::core::SchemaFrame::Paths &paths,
                           std::unordered_set<std::string> &visited) -> void {
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(schema, walker, resolver, default_dialect, default_id, paths);
  const auto origin{sourcemeta::core::identify(schema, resolver,
                                               default_dialect, default_id)};

  std::vector<
      std::tuple<sourcemeta::core::JSON, sourcemeta::core::JSON::String>>
      found;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) {
    // We don't want to report official schemas, as we can expect
    // virtually all implementations to understand them out of the box
    if (is_official_metaschema_reference(pointer, reference.destination)) {
      return;
    }

    if (reference.base.empty()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    // To not infinitely loop on circular references
    if (visited.contains(std::string{reference.base})) {
      return;
    }

    // If we can't find the destination but there is a base and we can
    // find the base, then we are facing an unresolved fragment
    if (frame.traverse(reference.base).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const auto &identifier{reference.base};
    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!sourcemeta::core::is_schema(remote.value())) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    const auto remote_base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!remote_base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    callback(origin, pointer, identifier, remote.value());
    found.emplace_back(std::move(remote).value(),
                       sourcemeta::core::JSON::String{identifier});
    visited.emplace(std::string{identifier});
  });

  for (const auto &entry : found) {
    dependencies_internal(std::get<0>(entry), walker, resolver, callback,
                          default_dialect, std::get<1>(entry),
                          {sourcemeta::core::empty_weak_pointer}, visited);
  }
}

auto embed_schema(sourcemeta::core::JSON &root,
                  const sourcemeta::core::Pointer &container,
                  const std::string_view identifier,
                  sourcemeta::core::JSON &&target) -> void {
  auto *current{&root};
  for (const auto &token : container) {
    if (token.is_property()) {
      current->assign_if_missing(token.to_property(),
                                 sourcemeta::core::JSON::make_object());
      current = &current->at(token.to_property());
    } else {
      assert(current->is_array() && current->size() >= token.to_index());
      current = &current->at(token.to_index());
    }
  }

  if (!current->is_object()) {
    throw sourcemeta::core::SchemaError(
        "Could not bundle to a container path that is not an object");
  }

  std::ostringstream key;
  key << identifier;
  // Ensure we get a definitions entry that does not exist
  while (current->defines(key.str())) {
    key << "/x";
  }

  current->assign(key.str(), std::move(target));
}

auto elevate_embedded_resources(
    sourcemeta::core::JSON &remote, sourcemeta::core::JSON &root,
    const sourcemeta::core::Pointer &container,
    const sourcemeta::core::SchemaBaseDialect remote_dialect,
    const sourcemeta::core::SchemaResolver &resolver,
    std::string_view default_dialect,
    std::unordered_map<sourcemeta::core::JSON::String,
                       sourcemeta::core::JSON::String> &bundled) -> void {
  const auto keyword{sourcemeta::core::definitions_keyword(remote_dialect)};
  const sourcemeta::core::JSON::String keyword_string{keyword};
  if (keyword.empty() || !remote.is_object() ||
      !remote.defines(keyword_string) ||
      !remote.at(keyword_string).is_object()) {
    return;
  }

  auto &defs{remote.at(keyword_string)};

  // Navigate to the root container once, as it doesn't change per entry
  const sourcemeta::core::JSON *root_container{&root};
  bool container_exists{true};
  for (const auto &token : container) {
    if (!token.is_property() || !root_container->is_object() ||
        !root_container->defines(token.to_property())) {
      container_exists = false;
      break;
    }

    root_container = &root_container->at(token.to_property());
  }

  std::vector<sourcemeta::core::JSON::String> to_extract;
  std::vector<sourcemeta::core::JSON::String> to_remove;
  for (const auto &entry : defs.as_object()) {
    const auto &key{entry.first};
    const auto &value{entry.second};
    const auto entry_dialect{
        sourcemeta::core::base_dialect(value, resolver, default_dialect)};
    const auto effective_entry_dialect{
        entry_dialect.has_value() ? entry_dialect.value() : remote_dialect};
    const auto identifier{
        sourcemeta::core::identify(value, effective_entry_dialect)};
    if (identifier.empty() || identifier != key ||
        !sourcemeta::core::URI{identifier}.is_absolute()) {
      continue;
    }

    const sourcemeta::core::JSON::String identifier_string{identifier};
    if (bundled.contains(identifier_string)) {
      if (container_exists && root_container->is_object()) {
        for (const auto &root_entry : root_container->as_object()) {
          if (!root_entry.first.starts_with(identifier_string)) {
            continue;
          }

          const auto stored_dialect{sourcemeta::core::base_dialect(
              root_entry.second, resolver, default_dialect)};
          const auto effective_stored_dialect{stored_dialect.has_value()
                                                  ? stored_dialect.value()
                                                  : remote_dialect};
          const auto stored_id{sourcemeta::core::identify(
              root_entry.second, effective_stored_dialect)};
          if (stored_id != identifier_string) {
            continue;
          }

          if (root_entry.second != value) {
            throw sourcemeta::core::SchemaError(
                "Conflicting embedded resources with the same identifier");
          }

          break;
        }
      }

      to_remove.emplace_back(key);
    } else {
      to_extract.emplace_back(key);
      bundled.emplace(identifier_string, identifier_string);
    }
  }

  for (const auto &key : to_extract) {
    auto value{std::move(defs.at(key))};
    defs.erase(key);
    embed_schema(root, container, key, std::move(value));
  }

  for (const auto &key : to_remove) {
    defs.erase(key);
  }

  if (defs.empty()) {
    remote.erase(sourcemeta::core::JSON::String{keyword});
  }
}

auto bundle_schema(sourcemeta::core::JSON &root,
                   const sourcemeta::core::Pointer &container,
                   sourcemeta::core::JSON &subschema,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const sourcemeta::core::SchemaFrame::Paths &paths,
                   std::unordered_map<sourcemeta::core::JSON::String,
                                      sourcemeta::core::JSON::String> &bundled,
                   const std::size_t depth = 0) -> void {
  // Create a fresh frame for each schema we analyze to avoid key collisions
  // between different schemas that have references at the same pointer paths
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  if (depth == 0) {
    frame.analyse(
        subschema, walker, resolver, default_dialect, default_id,
        // We only want to frame in "wrapper" mode for the top level object
        paths);
  } else {
    frame.analyse(subschema, walker, resolver, default_dialect, default_id);
  }

  std::vector<std::tuple<sourcemeta::core::JSON, sourcemeta::core::JSON::String,
                         sourcemeta::core::SchemaBaseDialect>>
      deferred;
  std::vector<
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::JSON::String>>
      ref_rewrites;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) {
    // We don't want to bundle official schemas, as we can expect
    // virtually all implementations to understand them out of the box
    if (is_official_metaschema_reference(pointer, reference.destination)) {
      return;
    }

    // If we can't find the destination but there is a base and we can
    // find base, then we are facing an unresolved fragment
    if (!reference.base.empty() && frame.traverse(reference.base).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    if (reference.base.empty()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const sourcemeta::core::JSON::String identifier{reference.base};

    if (bundled.contains(identifier)) {
      const auto &mapped_id{bundled.at(identifier)};
      if (mapped_id != identifier) {
        sourcemeta::core::URI rewrite_uri{mapped_id};
        if (reference.fragment.has_value()) {
          rewrite_uri.fragment(reference.fragment.value());
        }

        ref_rewrites.emplace_back(sourcemeta::core::to_pointer(pointer),
                                  rewrite_uri.recompose());
      }

      return;
    }

    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw sourcemeta::core::SchemaReferenceError(
            reference.destination, sourcemeta::core::to_pointer(pointer),
            "Could not resolve schema reference");
      }

      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!sourcemeta::core::is_schema(remote.value())) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    const auto remote_base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!remote_base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    auto remote_id =
        sourcemeta::core::identify(remote.value(), resolver, default_dialect);

    // If the reference has a fragment, verify it exists in the remote
    // schema
    if (reference.fragment.has_value()) {
      // TODO: The fact that we have to re-frame on each loop pass to check
      // for this is probably insanely slow
      sourcemeta::core::SchemaFrame remote_frame{
          sourcemeta::core::SchemaFrame::Mode::Locations};
      remote_frame.analyse(remote.value(), walker, resolver, default_dialect,
                           identifier);
      if (!remote_frame.traverse(reference.destination).has_value()) {
        throw sourcemeta::core::SchemaReferenceError(
            reference.destination, sourcemeta::core::to_pointer(pointer),
            "Could not resolve schema reference");
      }
    }

    sourcemeta::core::JSON::String effective_id{
        remote_id.empty() ? sourcemeta::core::JSON::String{identifier}
                          : sourcemeta::core::JSON::String{remote_id}};

    if (remote.value().is_object()) {
      sourcemeta::core::reidentify(remote.value(), effective_id,
                                   remote_base_dialect.value());
    }

    if (effective_id != identifier) {
      sourcemeta::core::URI rewrite_uri{effective_id};
      if (reference.fragment.has_value()) {
        rewrite_uri.fragment(reference.fragment.value());
      }

      ref_rewrites.emplace_back(sourcemeta::core::to_pointer(pointer),
                                rewrite_uri.recompose());
    }

    bundled.emplace(identifier, effective_id);
    bundled.emplace(effective_id, effective_id);
    deferred.emplace_back(std::move(remote).value(), std::move(effective_id),
                          remote_base_dialect.value());
  });

  for (auto &[rewrite_pointer, rewrite_value] : ref_rewrites) {
    sourcemeta::core::set(subschema, rewrite_pointer,
                          sourcemeta::core::JSON{rewrite_value});
  }

  for (auto &[remote, effective_id, remote_dialect] : deferred) {
    bundle_schema(root, container, remote, walker, resolver, default_dialect,
                  effective_id, paths, bundled, depth + 1);
    elevate_embedded_resources(remote, root, container, remote_dialect,
                               resolver, default_dialect, bundled);
    embed_schema(root, container, effective_id, std::move(remote));
  }
}

} // namespace

namespace sourcemeta::core {

auto dependencies(const JSON &schema, const SchemaWalker &walker,
                  const SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  std::string_view default_dialect, std::string_view default_id,
                  const SchemaFrame::Paths &paths) -> void {
  std::unordered_set<std::string> visited;
  dependencies_internal(schema, walker, resolver, callback, default_dialect,
                        default_id, paths, visited);
}

// TODO: Refactor this function to internally rely on the `.dependencies()`
// function
auto bundle(JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver, std::string_view default_dialect,
            std::string_view default_id,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> void {
  // Pre-scan the schema to find any already-embedded schemas and mark them
  // as bundled to avoid re-embedding them. This includes the root schema itself
  // and any schemas already embedded within it
  std::unordered_map<JSON::String, JSON::String> bundled;
  SchemaFrame initial_frame{SchemaFrame::Mode::Locations};
  initial_frame.analyse(schema, walker, resolver, default_dialect, default_id,
                        paths);
  initial_frame.for_each_resource_uri([&bundled](const auto uri) {
    bundled.emplace(JSON::String{uri}, JSON::String{uri});
  });
  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    bundle_schema(schema, default_container.value(), schema, walker, resolver,
                  default_dialect, default_id, paths, bundled);
    return;
  }

  // If the schema identifier is implicit, add it to the top-level of the
  // bundled schema. Otherwise, potential relative references based on this
  // implicit base URI will likely not resolve unless end users happen to
  // know that this implicit base URI is.
  if (!default_id.empty() &&
      identify(schema, resolver, default_dialect).empty()) {
    reidentify(schema, default_id, resolver, default_dialect);
  }

  const auto schema_base_dialect{
      base_dialect(schema, resolver, default_dialect)};
  if (!schema_base_dialect.has_value()) {
    throw SchemaError(
        "Could not determine how to perform bundling in this dialect");
  }

  const auto container_keyword{
      definitions_keyword(schema_base_dialect.value())};
  if (container_keyword.empty()) {
    SchemaFrame frame{SchemaFrame::Mode::References};
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
    if (frame.standalone()) {
      return;
    }

    throw SchemaError(
        "Could not determine how to perform bundling in this dialect");
  }

  if (ref_overrides_adjacent_keywords(schema_base_dialect.value()) &&
      schema.is_object() && schema.defines("$ref")) {
    if (schema.size() == 1) {
      auto branches{JSON::make_array()};
      branches.push_back(schema);
      schema.at("$ref").into(std::move(branches));
      schema.rename("$ref", "allOf");
    } else {
      throw SchemaError(
          "Cannot bundle a JSON Schema Draft 7 or older with a top-level "
          "`$ref` (which overrides sibling keywords) without introducing "
          "undefined behavior");
    }
  }

  bundle_schema(schema, {JSON::String{container_keyword}}, schema, walker,
                resolver, default_dialect, default_id, paths, bundled);
}

auto bundle(const JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver, std::string_view default_dialect,
            std::string_view default_id,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> JSON {
  JSON copy = schema;
  bundle(copy, walker, resolver, default_dialect, default_id, default_container,
         paths);
  return copy;
}

} // namespace sourcemeta::core
