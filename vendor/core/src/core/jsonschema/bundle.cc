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
         sourcemeta::core::schema_official_resolver(destination).has_value();
}

auto dependencies_internal(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::DependencyCallback &callback,
    const std::optional<std::string> &default_dialect,
    const std::optional<std::string> &default_id,
    const sourcemeta::core::SchemaFrame::Paths &paths,
    std::unordered_set<sourcemeta::core::JSON::String> &visited) -> void {
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(schema, walker, resolver, default_dialect, default_id, paths);
  const auto origin{sourcemeta::core::identify(
      schema, resolver, sourcemeta::core::SchemaIdentificationStrategy::Strict,
      default_dialect, default_id)};

  std::vector<
      std::tuple<sourcemeta::core::JSON,
                 std::reference_wrapper<const sourcemeta::core::JSON::String>>>
      found;

  for (const auto &[key, reference] : frame.references()) {
    if (frame.traverse(reference.destination).has_value() ||

        // We don't want to report official schemas, as we can expect
        // virtually all implementations to understand them out of the box
        is_official_metaschema_reference(key.second, reference.destination)) {
      continue;
    }

    if (!reference.base.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    // To not infinitely loop on circular references
    if (visited.contains(reference.base.value())) {
      continue;
    }

    // If we can't find the destination but there is a base and we can
    // find the base, then we are facing an unresolved fragment
    if (frame.traverse(reference.base.value()).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    assert(reference.base.has_value());
    const auto &identifier{reference.base.value()};
    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!sourcemeta::core::is_schema(remote.value())) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    const auto base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    callback(origin, key.second, identifier, remote.value());
    found.emplace_back(std::move(remote).value(), identifier);
    visited.emplace(identifier);
  }

  for (const auto &entry : found) {
    dependencies_internal(std::get<0>(entry), walker, resolver, callback,
                          default_dialect, std::get<1>(entry).get(),
                          {sourcemeta::core::empty_pointer}, visited);
  }
}

auto embed_schema(sourcemeta::core::JSON &root,
                  const sourcemeta::core::Pointer &container,
                  const std::string &identifier,
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
                   sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   const std::optional<std::string> &default_dialect,
                   const std::optional<std::string> &default_id,
                   const sourcemeta::core::SchemaFrame::Paths &paths,
                   const std::size_t depth = 0) -> void {
  // Keep in mind that the resulting frame does miss some information. For
  // example, when we recurse to framing embedded schemas, we will frame them
  // without keeping their new relationship to their parent (after embedding if
  // to the container location). However, that's fine for the purpose of this
  // function, given we don't pass the frame back to the caller
  if (depth == 0) {
    frame.analyse(
        subschema, walker, resolver, default_dialect, default_id,
        // We only want to frame in "wrapper" mode for the top level object
        paths);
  } else {
    frame.analyse(subschema, walker, resolver, default_dialect, default_id);
  }

  // Otherwise, given recursion, we would be modifying the
  // references list *while* looping on it
  // TODO: How can we avoid this very expensive copy?
  const auto references_copy = frame.references();
  for (const auto &[key, reference] : references_copy) {
    if (frame.traverse(reference.destination).has_value() ||

        // We don't want to bundle official schemas, as we can expect
        // virtually all implementations to understand them out of the box
        is_official_metaschema_reference(key.second, reference.destination)) {
      continue;
    }

    // If we can't find the destination but there is a base and we can
    // find base, then we are facing an unresolved fragment
    if (reference.base.has_value() &&
        frame.traverse(reference.base.value()).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    if (!reference.base.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    assert(reference.base.has_value());
    const auto &identifier{reference.base.value()};
    auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw sourcemeta::core::SchemaReferenceError(
            reference.destination, key.second,
            "Could not resolve schema reference");
      }

      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    if (!sourcemeta::core::is_schema(remote.value())) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    const auto base_dialect{sourcemeta::core::base_dialect(
        remote.value(), resolver, default_dialect)};
    if (!base_dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    if (remote.value().is_object()) {
      // Always insert an identifier, as a schema might refer to another schema
      // using another URI (i.e. due to relying on HTTP re-directions, etc)
      sourcemeta::core::reidentify(remote.value(), identifier,
                                   base_dialect.value());
    }

    bundle_schema(root, container, remote.value(), frame, walker, resolver,
                  default_dialect, identifier, paths, depth + 1);
    embed_schema(root, container, identifier, std::move(remote).value());
  }
}

} // namespace

namespace sourcemeta::core {

auto dependencies(const JSON &schema, const SchemaWalker &walker,
                  const SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  const std::optional<std::string> &default_dialect,
                  const std::optional<std::string> &default_id,
                  const SchemaFrame::Paths &paths) -> void {
  std::unordered_set<sourcemeta::core::JSON::String> visited;
  dependencies_internal(schema, walker, resolver, callback, default_dialect,
                        default_id, paths, visited);
}

// TODO: Refactor this function to internally rely on the `.dependencies()`
// function
auto bundle(JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect,
            const std::optional<std::string> &default_id,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> void {
  SchemaFrame frame{SchemaFrame::Mode::References};

  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    bundle_schema(schema, default_container.value(), schema, frame, walker,
                  resolver, default_dialect, default_id, paths);
    return;
  }

  const auto vocabularies{
      sourcemeta::core::vocabularies(schema, resolver, default_dialect)};
  if (vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/core") ||
      vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/core")) {
    bundle_schema(schema, {"$defs"}, schema, frame, walker, resolver,
                  default_dialect, default_id, paths);
    return;
  } else if (vocabularies.contains("http://json-schema.org/draft-07/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-07/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-06/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-06/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-04/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-04/hyper-schema#")) {
    bundle_schema(schema, {"definitions"}, schema, frame, walker, resolver,
                  default_dialect, default_id, paths);
    return;
  } else if (vocabularies.contains(
                 "http://json-schema.org/draft-03/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-03/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-02/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-02/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-01/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-01/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-00/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-00/schema#")) {
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
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect,
            const std::optional<std::string> &default_id,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> JSON {
  JSON copy = schema;
  bundle(copy, walker, resolver, default_dialect, default_id, default_container,
         paths);
  return copy;
}

} // namespace sourcemeta::core
