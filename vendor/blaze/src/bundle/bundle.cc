#include <sourcemeta/blaze/bundle.h>

#include <sourcemeta/blaze/foundation.h>

#include "helpers.h"

#include <cassert>       // assert
#include <functional>    // std::reference_wrapper
#include <optional>      // std::optional
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

namespace {

auto is_skippable_metaschema_reference(
    const sourcemeta::blaze::BundleMode mode,
    const sourcemeta::core::WeakPointer &pointer,
    const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  if (pointer.back().to_property() != "$schema") {
    return false;
  }

  return mode == sourcemeta::blaze::BundleMode::References ||
         sourcemeta::blaze::schema_is_official(destination);
}

auto dependencies_internal(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const sourcemeta::blaze::DependencyCallback &callback,
    std::string_view default_dialect, std::string_view default_id,
    const sourcemeta::blaze::SchemaFrame::Paths &paths,
    std::unordered_set<std::string> &visited) -> void {
  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::blaze::SchemaFrame::IdentifierMode::Additional,
      paths};
  const auto &origin{frame.root()};

  std::vector<
      std::tuple<sourcemeta::core::JSON, sourcemeta::core::JSON::String>>
      found;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) -> void {
    // We don't want to report official schemas, as we can expect
    // virtually all implementations to understand them out of the box
    if (is_skippable_metaschema_reference(
            sourcemeta::blaze::BundleMode::NonOfficialMetaschemas, pointer,
            reference.destination)) {
      return;
    }

    if (reference.base.empty()) {
      throw sourcemeta::blaze::SchemaReferenceError(
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
      throw sourcemeta::blaze::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const auto &identifier{reference.base};
    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      throw sourcemeta::blaze::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!remote.value().is_object() && !remote.value().is_boolean()) {
      throw sourcemeta::blaze::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    try {
      [[maybe_unused]] const sourcemeta::blaze::SchemaFrame remote_frame{
          sourcemeta::blaze::SchemaFrame::Mode::Root, remote.value(), walker,
          resolver, default_dialect};
    } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::blaze::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    callback(origin, pointer, identifier, remote.value());
    visited.emplace(identifier);

    // Official schemas can only reference other official schemas, so
    // recursing into them can never surface further dependencies
    if (sourcemeta::blaze::schema_is_official(identifier)) {
      return;
    }

    found.emplace_back(std::move(remote).value(),
                       sourcemeta::core::JSON::String{identifier});
  });

  for (const auto &entry : found) {
    dependencies_internal(std::get<0>(entry), walker, resolver, callback,
                          default_dialect, std::get<1>(entry),
                          {sourcemeta::core::EMPTY_WEAK_POINTER}, visited);
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
    throw sourcemeta::blaze::SchemaError(
        "Could not bundle to a container path that is not an object");
  }

  std::string key{identifier};
  // Ensure we get a definitions entry that does not exist
  while (current->defines(key)) {
    key += "/x";
  }

  current->assign(key, std::move(target));
}

auto elevate_embedded_resources(
    sourcemeta::core::JSON &remote, sourcemeta::core::JSON &root,
    const sourcemeta::core::Pointer &container,
    const sourcemeta::blaze::SchemaBaseDialect remote_dialect,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect,
    std::unordered_map<sourcemeta::core::JSON::String,
                       sourcemeta::core::JSON::String> &bundled) -> void {
  const auto keyword{sourcemeta::blaze::definitions_keyword(remote_dialect)};
  const sourcemeta::core::JSON::String keyword_string{keyword};
  if (keyword.empty() || !remote.is_object() ||
      !remote.defines(keyword_string) ||
      !remote.at(keyword_string).is_object()) {
    return;
  }

  auto &defs{remote.at(keyword_string)};
  const auto remote_dialect_uri{
      sourcemeta::blaze::declared_dialect(remote, default_dialect)};

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

  std::vector<std::pair<sourcemeta::core::JSON::String, bool>> to_extract;
  std::vector<sourcemeta::core::JSON::String> to_remove;
  for (const auto &entry : defs.as_object()) {
    const auto &key{entry.first};
    const auto &value{entry.second};
    // Only an entry that declares an absolute identifier matching its key can
    // ever be elevated, and framing rejects the fragment-only identifiers that
    // older drafts use for anchors. Rule those out before paying for a frame
    if (!value.is_object()) {
      continue;
    }
    const auto *declared_id{value.try_at("$id")};
    if (declared_id == nullptr) {
      declared_id = value.try_at("id");
    }
    if (declared_id == nullptr || !declared_id->is_string() ||
        declared_id->to_string() != key ||
        !sourcemeta::core::URI{declared_id->to_string()}.is_absolute()) {
      continue;
    }

    // The remote's dialect is what an entry that declares none inherits, so
    // hand it to the frame as the default rather than falling back after
    sourcemeta::blaze::SchemaFrame entry_frame{
        sourcemeta::blaze::SchemaFrame::Mode::Root, value, walker, resolver,
        remote_dialect_uri};
    const auto &identifier{entry_frame.root()};
    if (identifier.empty() || identifier != key ||
        !sourcemeta::core::URI{identifier}.is_absolute()) {
      continue;
    }

    const sourcemeta::core::JSON::String identifier_string{identifier};
    const auto defines_dialect{value.defines("$schema")};
    if (bundled.contains(identifier_string)) {
      if (container_exists && root_container->is_object()) {
        for (const auto &root_entry : root_container->as_object()) {
          if (!root_entry.first.starts_with(identifier_string)) {
            continue;
          }

          // Same reasoning as above: rule out what cannot match, and what
          // framing would reject, before paying for a frame
          if (!root_entry.second.is_object()) {
            continue;
          }
          const auto *stored_declared_id{root_entry.second.try_at("$id")};
          if (stored_declared_id == nullptr) {
            stored_declared_id = root_entry.second.try_at("id");
          }
          if (stored_declared_id == nullptr ||
              !stored_declared_id->is_string() ||
              stored_declared_id->to_string() != identifier_string ||
              !sourcemeta::core::URI{stored_declared_id->to_string()}
                   .is_absolute()) {
            continue;
          }

          sourcemeta::blaze::SchemaFrame stored_frame{
              sourcemeta::blaze::SchemaFrame::Mode::Root, root_entry.second,
              walker, resolver, remote_dialect_uri};
          const auto &stored_id{stored_frame.root()};
          if (stored_id != identifier_string) {
            continue;
          }

          if (defines_dialect) {
            if (root_entry.second != value) {
              throw sourcemeta::blaze::SchemaError(
                  "Conflicting embedded resources with the same identifier");
            }
          } else {
            // The stored copy of the resource got its dialect stamped on
            // extraction, so compare against a candidate that is stamped in
            // the same way
            auto candidate{value};
            candidate.assign("$schema", sourcemeta::core::JSON{
                                            sourcemeta::blaze::declared_dialect(
                                                value, remote_dialect_uri)});
            if (root_entry.second != candidate) {
              throw sourcemeta::blaze::SchemaError(
                  "Conflicting embedded resources with the same identifier");
            }
          }

          break;
        }
      }

      to_remove.emplace_back(key);
    } else {
      to_extract.emplace_back(key, !defines_dialect);
      bundled.emplace(identifier_string, identifier_string);
    }
  }

  for (const auto &[key, needs_dialect] : to_extract) {
    auto value{std::move(defs.at(key))};
    defs.erase(key);
    // Otherwise the elevated resource would be re-interpreted under the
    // dialect of the schema it gets embedded into, which can differ from
    // the dialect it inherited from the remote it was elevated out of
    if (needs_dialect) {
      value.assign("$schema",
                   sourcemeta::core::JSON{sourcemeta::blaze::declared_dialect(
                       value, remote_dialect_uri)});
    }

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
                   const sourcemeta::blaze::SchemaWalker &walker,
                   const sourcemeta::blaze::SchemaResolver &resolver,
                   const sourcemeta::blaze::BundleMode mode,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const sourcemeta::blaze::SchemaFrame::Paths &paths,
                   std::unordered_map<sourcemeta::core::JSON::String,
                                      sourcemeta::core::JSON::String> &bundled,
                   const std::size_t depth = 0) -> void {
  // Create a fresh frame for each schema we analyze to avoid key collisions
  // between different schemas that have references at the same pointer paths
  static const sourcemeta::blaze::SchemaFrame::Paths NESTED_PATHS{
      sourcemeta::core::EMPTY_WEAK_POINTER};
  const sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References, subschema, walker,
      resolver, default_dialect, default_id,
      sourcemeta::blaze::SchemaFrame::IdentifierMode::Additional,
      // We only want to frame in "wrapper" mode for the top level object
      depth == 0 ? paths : NESTED_PATHS};

  std::vector<std::tuple<sourcemeta::core::JSON, sourcemeta::core::JSON::String,
                         sourcemeta::blaze::SchemaBaseDialect>>
      deferred;
  std::vector<
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::JSON::String>>
      ref_rewrites;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) -> void {
    // We don't want to bundle official schemas, as we can expect
    // virtually all implementations to understand them out of the box.
    // Depending on the bundling strategy, we may skip meta-schemas entirely
    if (is_skippable_metaschema_reference(mode, pointer,
                                          reference.destination)) {
      return;
    }

    // If we can't find the destination but there is a base and we can
    // find base, then we are facing an unresolved fragment
    if (!reference.base.empty() && frame.traverse(reference.base).has_value()) {
      throw sourcemeta::blaze::SchemaReferenceError(
          reference.destination, sourcemeta::core::to_pointer(pointer),
          "Could not resolve schema reference");
    }

    if (reference.base.empty()) {
      throw sourcemeta::blaze::SchemaReferenceError(
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

    auto resolved{resolver(identifier)};
    if (!resolved.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw sourcemeta::blaze::SchemaReferenceError(
            reference.destination, sourcemeta::core::to_pointer(pointer),
            "Could not resolve schema reference");
      }

      throw sourcemeta::blaze::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    // Bundling rewrites the schema before embedding it, so it needs a copy
    // it owns rather than whatever the resolver chose to hand back
    auto remote{std::move(resolved).to_owned()};
    if (!remote.is_object() && !remote.is_boolean()) {
      throw sourcemeta::blaze::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    std::optional<sourcemeta::blaze::SchemaFrame> remote_root_frame;
    try {
      remote_root_frame.emplace(sourcemeta::blaze::SchemaFrame::Mode::Root,
                                remote, walker, resolver, default_dialect);
    } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::blaze::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    const auto remote_base_dialect{
        remote_root_frame->root_location().value().get().base_dialect};
    auto remote_id = remote_root_frame->root();

    // If the reference has a fragment, verify it exists in the remote
    // schema
    if (reference.fragment.has_value()) {
      // TODO: The fact that we have to re-frame on each loop pass to check
      // for this is probably insanely slow
      sourcemeta::blaze::SchemaFrame remote_frame{
          sourcemeta::blaze::SchemaFrame::Mode::Locations,
          remote,
          walker,
          resolver,
          default_dialect,
          identifier};
      if (!remote_frame.traverse(reference.destination).has_value()) {
        throw sourcemeta::blaze::SchemaReferenceError(
            reference.destination, sourcemeta::core::to_pointer(pointer),
            "Could not resolve schema reference");
      }
    }

    sourcemeta::core::JSON::String effective_id{
        remote_id.empty() ? sourcemeta::core::JSON::String{identifier}
                          : sourcemeta::core::JSON::String{remote_id}};

    if (remote.is_object()) {
      // Otherwise the embedded resource would be re-interpreted under the
      // dialect of the schema it gets embedded into, which can differ from
      // the default dialect that the remote was resolved with
      if (!remote.defines("$schema")) {
        remote.assign("$schema", sourcemeta::core::JSON{
                                     sourcemeta::blaze::declared_dialect(
                                         remote, default_dialect)});
      }

      sourcemeta::blaze::schema_reidentify(remote, effective_id,
                                           remote_base_dialect);
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
    deferred.emplace_back(std::move(remote), std::move(effective_id),
                          remote_base_dialect);
  });

  for (auto &[rewrite_pointer, rewrite_value] : ref_rewrites) {
    sourcemeta::core::set(subschema, rewrite_pointer,
                          sourcemeta::core::JSON{rewrite_value});
  }

  for (auto &[remote, effective_id, remote_dialect] : deferred) {
    bundle_schema(root, container, remote, walker, resolver, mode,
                  default_dialect, effective_id, paths, bundled, depth + 1);
    elevate_embedded_resources(remote, root, container, remote_dialect, walker,
                               resolver, default_dialect, bundled);
    embed_schema(root, container, effective_id, std::move(remote));
  }
}

} // namespace

namespace sourcemeta::blaze {

auto dependencies(const sourcemeta::core::JSON &schema,
                  const SchemaWalker &walker, const SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  std::string_view default_dialect, std::string_view default_id,
                  const SchemaFrame::Paths &paths) -> void {
  std::unordered_set<std::string> visited;
  dependencies_internal(schema, walker, resolver, callback, default_dialect,
                        default_id, paths, visited);
}

// TODO: Refactor this function to internally rely on the `.dependencies()`
// function
auto bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver, const BundleMode mode,
            std::string_view default_dialect, std::string_view default_id,
            const std::optional<sourcemeta::core::Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> void {
  // Pre-scan the schema to find any already-embedded schemas and mark them
  // as bundled to avoid re-embedding them. This includes the root schema itself
  // and any schemas already embedded within it
  std::unordered_map<sourcemeta::core::JSON::String,
                     sourcemeta::core::JSON::String>
      bundled;
  SchemaFrame initial_frame{
      SchemaFrame::Mode::Locations,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::blaze::SchemaFrame::IdentifierMode::Additional,
      paths};
  initial_frame.for_each_resource_uri([&bundled](const auto &uri) -> void {
    bundled.emplace(sourcemeta::core::JSON::String{uri},
                    sourcemeta::core::JSON::String{uri});
  });
  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    bundle_schema(schema, default_container.value(), schema, walker, resolver,
                  mode, default_dialect, default_id, paths, bundled);
    return;
  }

  // If the schema identifier is implicit, add it to the top-level of the
  // bundled schema. Otherwise, potential relative references based on this
  // implicit base URI will likely not resolve unless end users happen to
  // know that this implicit base URI is. Note that boolean schemas cannot
  // declare identifiers, so we leave those untouched
  if (!default_id.empty() && schema.is_object()) {
    // Deliberately framed without a default identifier, so that the root
    // comes back empty exactly when the schema declares none of its own
    SchemaFrame declared_frame{SchemaFrame::Mode::Root, schema, walker,
                               resolver, default_dialect};
    if (declared_frame.root().empty()) {
      schema_reidentify(schema, default_id, resolver, default_dialect);
    }
  }

  std::optional<SchemaFrame> schema_root_frame;
  try {
    schema_root_frame.emplace(SchemaFrame::Mode::Root, schema, walker, resolver,
                              default_dialect, default_id);
  } catch (const SchemaUnknownBaseDialectError &) {
    throw SchemaError(
        "Could not determine how to perform bundling in this dialect");
  }

  const auto schema_base_dialect{
      schema_root_frame->root_location().value().get().base_dialect};

  const auto container_keyword{definitions_keyword(schema_base_dialect)};
  if (container_keyword.empty()) {
    SchemaFrame frame{SchemaFrame::Mode::References,
                      schema,
                      walker,
                      resolver,
                      default_dialect,
                      default_id};
    if (frame.standalone()) {
      return;
    }

    throw SchemaError(
        "Could not determine how to perform bundling in this dialect");
  }

  if (ref_overrides_adjacent_keywords(schema_base_dialect) &&
      schema.is_object() && schema.defines("$ref")) {
    if (schema.size() == 1) {
      const auto is_draft3{
          schema_base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3 ||
          schema_base_dialect == SchemaBaseDialect::JSON_Schema_Draft_3_Hyper};
      auto branches{sourcemeta::core::JSON::make_array()};
      branches.push_back(schema);
      schema.at("$ref").into(std::move(branches));
      schema.rename("$ref", is_draft3 ? "extends" : "allOf");
    } else {
      throw SchemaError(
          "Cannot bundle a JSON Schema Draft 7 or older with a top-level "
          "`$ref` (which overrides sibling keywords) without introducing "
          "undefined behavior");
    }
  }

  bundle_schema(schema, {sourcemeta::core::JSON::String{container_keyword}},
                schema, walker, resolver, mode, default_dialect, default_id,
                paths, bundled);
}

auto bundle(const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver, const BundleMode mode,
            std::string_view default_dialect, std::string_view default_id,
            const std::optional<sourcemeta::core::Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> sourcemeta::core::JSON {
  sourcemeta::core::JSON copy = schema;
  bundle(copy, walker, resolver, mode, default_dialect, default_id,
         default_container, paths);
  return copy;
}

} // namespace sourcemeta::blaze
