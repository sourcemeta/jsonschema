#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/uri.h>

#include "helpers.h"

#include <algorithm>     // std::ranges::any_of
#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint64_t
#include <functional>    // std::cref
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

auto is_skippable_metaschema_reference(const SchemaBundleOptions::Mode mode,
                                       const WeakPointer &pointer,
                                       const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  if (pointer.back().to_property() != "$schema") {
    return false;
  }

  return mode == SchemaBundleOptions::Mode::References ||
         schema_is_official(destination);
}

// The dialect a schema declares, falling back to the given default
auto declared_dialect(const JSON &schema,
                      const std::string_view default_dialect)
    -> std::string_view {
  if (!schema.is_object()) {
    return default_dialect;
  }

  const auto *dialect{schema.try_at("$schema")};
  return (dialect != nullptr && dialect->is_string()) ? dialect->to_string()
                                                      : default_dialect;
}

// Every frame that bundling constructs spends from the same limit, as how
// many frames it ends up needing is a function of what the resolver hands
// back rather than of the schema the caller passed in. A frame that ran past
// what was left of the limit threw rather than returned, so what it holds is
// always within it
auto charge(std::uint64_t &remaining, const SchemaFrame &frame) -> void {
  assert(frame.location_count() <= remaining);
  remaining -= frame.location_count();
}

auto embed_schema(JSON &root, const Pointer &container,
                  const std::string_view identifier, JSON &&target,
                  const SchemaBundleOptions::Callback &callback) -> void {
  auto *current{&root};
  for (const auto &token : container) {
    if (token.is_property()) {
      current->assign_if_missing(token.to_property(), JSON::make_object());
      current = &current->at(token.to_property());
    } else {
      assert(current->is_array() && current->size() >= token.to_index());
      current = &current->at(token.to_index());
    }
  }

  if (!current->is_object()) {
    throw SchemaError("Could not bundle to a container path that is not an "
                      "object");
  }

  std::string key{identifier};
  // Ensure we get a definitions entry that does not exist
  while (current->defines(key)) {
    key += "/x";
  }

  current->assign(key, std::move(target));

  if (callback) {
    auto location{to_weak_pointer(container)};
    location.push_back(std::cref(key));
    callback(identifier, location);
  }
}

auto elevate_embedded_resources(
    JSON &remote, JSON &root, const Pointer &container,
    const SchemaBaseDialect remote_dialect, const SchemaWalker &walker,
    const SchemaResolver &resolver, std::string_view default_dialect,
    std::unordered_map<JSON::String, JSON::String> &bundled,
    std::uint64_t &remaining, const SchemaBundleOptions::Callback &callback)
    -> void {
  const auto keyword{definitions_keyword(remote_dialect)};
  const JSON::String keyword_string{keyword};
  if (keyword.empty() || !remote.is_object() ||
      !remote.defines(keyword_string) ||
      !remote.at(keyword_string).is_object()) {
    return;
  }

  auto &defs{remote.at(keyword_string)};
  const auto remote_dialect_uri{declared_dialect(remote, default_dialect)};

  // Navigate to the root container once, as it doesn't change per entry
  const JSON *root_container{&root};
  bool container_exists{true};
  for (const auto &token : container) {
    if (!token.is_property() || !root_container->is_object() ||
        !root_container->defines(token.to_property())) {
      container_exists = false;
      break;
    }

    root_container = &root_container->at(token.to_property());
  }

  std::vector<std::pair<JSON::String, bool>> to_extract;
  std::vector<JSON::String> to_remove;
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
        !URI{declared_id->to_string()}.is_absolute()) {
      continue;
    }

    // The remote's dialect is what an entry that declares none inherits, so
    // hand it to the frame as the default rather than falling back after
    SchemaFrame entry_frame{SchemaFrame::Mode::Root,
                            value,
                            walker,
                            resolver,
                            remote_dialect_uri,
                            "",
                            SchemaFrame::IdentifierMode::Additional,
                            {EMPTY_WEAK_POINTER},
                            "",
                            remaining};
    charge(remaining, entry_frame);
    const auto &identifier{entry_frame.root()};
    if (identifier.empty() || identifier != key ||
        !URI{identifier}.is_absolute()) {
      continue;
    }

    const JSON::String identifier_string{identifier};
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
              !URI{stored_declared_id->to_string()}.is_absolute()) {
            continue;
          }

          SchemaFrame stored_frame{SchemaFrame::Mode::Root,
                                   root_entry.second,
                                   walker,
                                   resolver,
                                   remote_dialect_uri,
                                   "",
                                   SchemaFrame::IdentifierMode::Additional,
                                   {EMPTY_WEAK_POINTER},
                                   "",
                                   remaining};
          charge(remaining, stored_frame);
          const auto &stored_id{stored_frame.root()};
          if (stored_id != identifier_string) {
            continue;
          }

          if (defines_dialect) {
            if (root_entry.second != value) {
              throw SchemaError(
                  "Conflicting embedded resources with the same identifier");
            }
          } else {
            // The stored copy of the resource got its dialect stamped on
            // extraction, so compare against a candidate that is stamped in
            // the same way
            auto candidate{value};
            candidate.assign("$schema",
                             JSON{declared_dialect(value, remote_dialect_uri)});
            if (root_entry.second != candidate) {
              throw SchemaError(
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
                   JSON{declared_dialect(value, remote_dialect_uri)});
    }

    embed_schema(root, container, key, std::move(value), callback);
  }

  for (const auto &key : to_remove) {
    defs.erase(key);
  }

  if (defs.empty()) {
    remote.erase(JSON::String{keyword});
  }
}

auto embed_references(
    JSON &root, const Pointer &container, JSON &subschema,
    const SchemaWalker &walker, const SchemaResolver &resolver,
    const SchemaBundleOptions::Mode mode, std::string_view default_dialect,
    std::string_view default_id, const SchemaFrame::Paths &paths,
    std::string_view default_base,
    std::unordered_map<JSON::String, JSON::String> &bundled,
    std::uint64_t &remaining, const SchemaBundleOptions::Callback &callback,
    const std::size_t depth = 0) -> void {
  // Create a fresh frame for each schema we analyze to avoid key collisions
  // between different schemas that have references at the same pointer paths
  static const SchemaFrame::Paths NESTED_PATHS{EMPTY_WEAK_POINTER};
  const SchemaFrame frame{
      SchemaFrame::Mode::References, subschema, walker, resolver,
      default_dialect, default_id, SchemaFrame::IdentifierMode::Additional,
      // We only want to frame in "wrapper" mode for the top
      // level object, which is also the only one that the
      // base the caller retrieved it from applies to, as
      // every remote carries the identity it was resolved by
      depth == 0 ? paths : NESTED_PATHS,
      depth == 0 ? default_base : std::string_view{}, remaining};
  charge(remaining, frame);

  std::vector<std::tuple<JSON, JSON::String, SchemaBaseDialect>> deferred;
  std::vector<std::pair<Pointer, JSON::String>> ref_rewrites;

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
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    if (reference.base.empty()) {
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const JSON::String identifier{reference.base};

    if (bundled.contains(identifier)) {
      const auto &mapped_id{bundled.at(identifier)};
      if (mapped_id != identifier) {
        URI rewrite_uri{mapped_id};
        if (reference.fragment.has_value()) {
          rewrite_uri.fragment(reference.fragment.value());
        }

        ref_rewrites.emplace_back(to_pointer(pointer), rewrite_uri.recompose());
      }

      return;
    }

    auto resolved{resolver(identifier)};
    if (!resolved.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                   "Could not resolve schema reference");
      }

      throw SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    // Bundling rewrites the schema before embedding it, so it needs a copy
    // it owns rather than whatever the resolver chose to hand back
    auto remote{std::move(resolved).to_owned()};
    if (!remote.is_object() && !remote.is_boolean()) {
      throw SchemaReferenceError(identifier, to_pointer(pointer),
                                 "The JSON document is not a valid JSON "
                                 "Schema");
    }

    std::optional<SchemaFrame> remote_root_frame;
    try {
      remote_root_frame.emplace(
          SchemaFrame::Mode::Root, remote, walker, resolver, default_dialect,
          "", SchemaFrame::IdentifierMode::Additional,
          SchemaFrame::Paths{EMPTY_WEAK_POINTER}, "", remaining);
      charge(remaining, remote_root_frame.value());
    } catch (const SchemaUnknownBaseDialectError &) {
      throw SchemaReferenceError(identifier, to_pointer(pointer),
                                 "The JSON document is not a valid JSON "
                                 "Schema");
    }

    const auto remote_base_dialect{
        remote_root_frame->root_location().value().get().base_dialect};
    auto remote_id = remote_root_frame->root();

    // If the reference has a fragment, verify it exists in the remote
    // schema
    if (reference.fragment.has_value()) {
      // A pointer fragment names a place of the document, and the document can
      // answer for that on its own without paying to frame it
      const auto fragment_pointer{
          fragment_to_pointer(URI{reference.destination})};
      bool exists{fragment_pointer.has_value() &&
                  try_get(remote, fragment_pointer.value()) != nullptr};

      // An anchor is not a place of the document, and the drafts that spell
      // identifiers as `id` let one look just like a pointer, so a miss above
      // still has to ask the frame. Only the anchors of the remote matter
      // here, rather than every pointer of it
      if (!exists) {
        const SchemaFrame remote_frame{SchemaFrame::Mode::Locations,
                                       remote,
                                       walker,
                                       resolver,
                                       default_dialect,
                                       identifier,
                                       SchemaFrame::IdentifierMode::Additional,
                                       {EMPTY_WEAK_POINTER},
                                       "",
                                       remaining};
        charge(remaining, remote_frame);
        exists = remote_frame.traverse(reference.destination).has_value();
      }

      if (!exists) {
        throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                   "Could not resolve schema reference");
      }
    }

    JSON::String effective_id{remote_id.empty() ? JSON::String{identifier}
                                                : JSON::String{remote_id}};

    if (remote.is_object()) {
      // Otherwise the embedded resource would be re-interpreted under the
      // dialect of the schema it gets embedded into, which can differ from
      // the default dialect that the remote was resolved with
      if (!remote.defines("$schema")) {
        remote.assign("$schema",
                      JSON{declared_dialect(remote, default_dialect)});
      }

      schema_reidentify(remote, effective_id, remote_base_dialect);
    }

    if (effective_id != identifier) {
      URI rewrite_uri{effective_id};
      if (reference.fragment.has_value()) {
        rewrite_uri.fragment(reference.fragment.value());
      }

      ref_rewrites.emplace_back(to_pointer(pointer), rewrite_uri.recompose());
    }

    bundled.emplace(identifier, effective_id);
    bundled.emplace(effective_id, effective_id);
    deferred.emplace_back(std::move(remote), std::move(effective_id),
                          remote_base_dialect);
  });

  for (auto &[rewrite_pointer, rewrite_value] : ref_rewrites) {
    set(subschema, rewrite_pointer, JSON{rewrite_value});
  }

  for (auto &[remote, effective_id, remote_dialect] : deferred) {
    embed_references(root, container, remote, walker, resolver, mode,
                     default_dialect, effective_id, paths, default_base,
                     bundled, remaining, callback, depth + 1);
    elevate_embedded_resources(remote, root, container, remote_dialect, walker,
                               resolver, default_dialect, bundled, remaining,
                               callback);
    embed_schema(root, container, effective_id, std::move(remote), callback);
  }
}

auto bundle_internal(JSON &schema, const SchemaWalker &walker,
                     const SchemaResolver &resolver,
                     const SchemaBundleOptions::Mode mode,
                     std::string_view default_dialect,
                     std::string_view default_id,
                     const std::optional<Pointer> &default_container,
                     const SchemaFrame::Paths &paths,
                     std::string_view default_base, std::uint64_t &remaining,
                     const SchemaBundleOptions::Callback &callback) -> void {
  // Pre-scan the schema to find any already-embedded schemas and mark them
  // as bundled to avoid re-embedding them. This includes the root schema itself
  // and any schemas already embedded within it
  std::unordered_map<JSON::String, JSON::String> bundled;
  SchemaFrame initial_frame{SchemaFrame::Mode::Locations,
                            schema,
                            walker,
                            resolver,
                            default_dialect,
                            default_id,
                            SchemaFrame::IdentifierMode::Additional,
                            paths,
                            default_base,
                            remaining};
  charge(remaining, initial_frame);
  initial_frame.for_each_resource_uri([&bundled](const auto &uri) -> void {
    bundled.emplace(JSON::String{uri}, JSON::String{uri});
  });
  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    // Whatever bundling embeds has to land somewhere that framing reaches
    // again, or a later pass cannot see it and embeds a second copy. So a
    // container has to be a keyword that the dialect reserves for schema
    // definitions, declared on a schema that the given paths cover. A wrapper
    // format keeps its container outside every schema it frames, where JSON
    // Schema has nothing to say about where things may go
    const auto container{to_weak_pointer(default_container.value())};
    if (std::ranges::any_of(paths, [&container](const auto &path) -> bool {
          return container.starts_with(path);
        })) {
      const auto parent_pointer{default_container.value().initial()};
      const auto parent{
          initial_frame.traverse(to_weak_pointer(parent_pointer))};
      if (!parent.has_value() ||
          (parent.value().get().type != SchemaFrame::LocationType::Resource &&
           parent.value().get().type != SchemaFrame::LocationType::Subschema) ||
          !default_container.value().back().is_property() ||
          walker(default_container.value().back().to_property(),
                 initial_frame.vocabularies(parent.value().get(), resolver))
                  .type != SchemaKeywordType::LocationMembers) {
        throw SchemaError("Could not bundle to a container that the dialect "
                          "does not reserve for schema definitions");
      }
    }

    embed_references(schema, default_container.value(), schema, walker,
                     resolver, mode, default_dialect, default_id, paths,
                     default_base, bundled, remaining, callback);
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
    SchemaFrame declared_frame{SchemaFrame::Mode::Root,
                               schema,
                               walker,
                               resolver,
                               default_dialect,
                               "",
                               SchemaFrame::IdentifierMode::Additional,
                               {EMPTY_WEAK_POINTER},
                               "",
                               remaining};
    charge(remaining, declared_frame);
    if (declared_frame.root().empty()) {
      schema_reidentify(schema, default_id, resolver, default_dialect);
    }
  }

  std::optional<SchemaFrame> schema_root_frame;
  try {
    schema_root_frame.emplace(
        SchemaFrame::Mode::Root, schema, walker, resolver, default_dialect,
        default_id, SchemaFrame::IdentifierMode::Additional,
        SchemaFrame::Paths{EMPTY_WEAK_POINTER}, "", remaining);
    charge(remaining, schema_root_frame.value());
  } catch (const SchemaUnknownBaseDialectError &) {
    throw SchemaError("Could not determine how to perform bundling in this "
                      "dialect");
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
                      default_id,
                      SchemaFrame::IdentifierMode::Additional,
                      {EMPTY_WEAK_POINTER},
                      default_base,
                      remaining};
    charge(remaining, frame);
    if (frame.standalone()) {
      return;
    }

    throw SchemaError("Could not determine how to perform bundling in this "
                      "dialect");
  }

  if (ref_overrides_adjacent_keywords(schema_base_dialect) &&
      schema.is_object() && schema.defines("$ref")) {
    if (schema.size() == 1) {
      const auto is_draft3{
          schema_base_dialect == SchemaBaseDialect::JSON_SCHEMA_DRAFT_3 ||
          schema_base_dialect == SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER};
      auto branches{JSON::make_array()};
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

  embed_references(schema, {JSON::String{container_keyword}}, schema, walker,
                   resolver, mode, default_dialect, default_id, paths,
                   default_base, bundled, remaining, callback);
}

} // namespace

auto schema_bundle(JSON &schema, const SchemaWalker &walker,
                   const SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const SchemaBundleOptions &options) -> void {
  auto remaining{options.max_locations};
  try {
    bundle_internal(schema, walker, resolver, options.mode, default_dialect,
                    default_id, options.default_container, options.paths,
                    options.default_base, remaining, options.callback);
  } catch (const SchemaFrameLimitError &) {
    // Every frame spends from what is left rather than from the whole, so the
    // one that ran out reports what it was handed. The caller set the limit
    // for the operation, so that is what the operation reports back
    throw SchemaFrameLimitError{options.max_locations};
  }
}

auto schema_bundle(const JSON &schema, const SchemaWalker &walker,
                   const SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const SchemaBundleOptions &options) -> JSON {
  JSON copy = schema;
  schema_bundle(copy, walker, resolver, default_dialect, default_id, options);
  return copy;
}

} // namespace sourcemeta::core
