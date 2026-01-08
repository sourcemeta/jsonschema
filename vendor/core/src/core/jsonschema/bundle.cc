#include <sourcemeta/core/jsonschema.h>

#include <cassert>       // assert
#include <functional>    // std::reference_wrapper
#include <sstream>       // std::ostringstream
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace {

auto is_official_metaschema_reference(const sourcemeta::core::Pointer &pointer,
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
                           std::unordered_set<std::string_view> &visited)
    -> void {
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
          reference.destination, pointer, "Could not resolve schema reference");
    }

    // To not infinitely loop on circular references
    if (visited.contains(reference.base)) {
      return;
    }

    // If we can't find the destination but there is a base and we can
    // find the base, then we are facing an unresolved fragment
    if (frame.traverse(reference.base).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, pointer, "Could not resolve schema reference");
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
          identifier, pointer, "The JSON document is not a valid JSON Schema");
    }

    const auto remote_base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!remote_base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, pointer, "The JSON document is not a valid JSON Schema");
    }

    callback(origin, pointer, identifier, remote.value());
    found.emplace_back(std::move(remote).value(),
                       sourcemeta::core::JSON::String{identifier});
    visited.emplace(identifier);
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

auto bundle_schema(sourcemeta::core::JSON &root,
                   const sourcemeta::core::Pointer &container,
                   const sourcemeta::core::JSON &subschema,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const sourcemeta::core::SchemaFrame::Paths &paths,
                   std::unordered_set<sourcemeta::core::JSON::String> &bundled,
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
          reference.destination, pointer, "Could not resolve schema reference");
    }

    if (reference.base.empty()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, pointer, "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const sourcemeta::core::JSON::String identifier{reference.base};

    // Skip if already bundled to avoid infinite loops on circular
    // references
    if (bundled.contains(identifier)) {
      return;
    }

    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw sourcemeta::core::SchemaReferenceError(
            reference.destination, pointer,
            "Could not resolve schema reference");
      }

      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!sourcemeta::core::is_schema(remote.value())) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, pointer, "The JSON document is not a valid JSON Schema");
    }

    const auto remote_base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!remote_base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, pointer, "The JSON document is not a valid JSON Schema");
    }

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
            reference.destination, pointer,
            "Could not resolve schema reference");
      }
    }

    if (remote.value().is_object()) {
      // Always insert an identifier, as a schema might refer to another
      // schema using another URI (i.e. due to relying on HTTP
      // re-directions, etc)
      sourcemeta::core::reidentify(remote.value(), identifier,
                                   remote_base_dialect.value());
    }

    bundled.emplace(identifier);
    bundle_schema(root, container, remote.value(), walker, resolver,
                  default_dialect, identifier, paths, bundled, depth + 1);
    embed_schema(root, container, identifier, std::move(remote).value());
  });
}

} // namespace

namespace sourcemeta::core {

auto dependencies(const JSON &schema, const SchemaWalker &walker,
                  const SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  std::string_view default_dialect, std::string_view default_id,
                  const SchemaFrame::Paths &paths) -> void {
  std::unordered_set<std::string_view> visited;
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
  std::unordered_set<JSON::String> bundled;
  SchemaFrame initial_frame{SchemaFrame::Mode::Locations};
  initial_frame.analyse(schema, walker, resolver, default_dialect, default_id,
                        paths);
  initial_frame.for_each_resource_uri(
      [&bundled](const auto uri) { bundled.emplace(uri); });
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

  const auto vocabularies{
      sourcemeta::core::vocabularies(schema, resolver, default_dialect)};
  if (vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_2020_12_Core) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_2019_09_Core)) {
    bundle_schema(schema, {"$defs"}, schema, walker, resolver, default_dialect,
                  default_id, paths, bundled);
    return;
  } else if (
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_7) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_7_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_6) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_6_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_4) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_4_Hyper)) {
    if (schema.is_object() && schema.defines("$ref")) {
      // This is a very specific case in which we can "fix" this
      if (schema.size() == 1) {
        auto branches{JSON::make_array()};
        branches.push_back(schema);
        schema.at("$ref").into(std::move(branches));
        // Note that `allOf` was introduced in Draft 4
        schema.rename("$ref", "allOf");
      } else {
        throw sourcemeta::core::SchemaError(
            "Cannot bundle a JSON Schema Draft 7 or older with a top-level "
            "`$ref` (which overrides sibling keywords) without introducing "
            "undefined behavior");
      }
    }

    bundle_schema(schema, {"definitions"}, schema, walker, resolver,
                  default_dialect, default_id, paths, bundled);
    return;
  } else if (
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_3_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_3) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_2_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_2) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_1_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_1) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_0_Hyper) ||
      vocabularies.contains(
          sourcemeta::core::Vocabularies::Known::JSON_Schema_Draft_0)) {
    SchemaFrame frame{SchemaFrame::Mode::References};
    frame.analyse(schema, walker, resolver, default_dialect, default_id);
    if (frame.standalone()) {
      return;
    }
  }

  // We don't attempt to bundle on dialects where we
  // don't know where to put the embedded schemas
  throw SchemaError(
      "Could not determine how to perform bundling in this dialect");
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
