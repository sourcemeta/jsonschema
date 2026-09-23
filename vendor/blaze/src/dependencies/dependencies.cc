#include <sourcemeta/blaze/dependencies.h>

#include <sourcemeta/core/jsonschema.h>

#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace {

auto is_skippable_metaschema_reference(
    const sourcemeta::core::WeakPointer &pointer,
    const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  if (pointer.back().to_property() != "$schema") {
    return false;
  }

  return sourcemeta::core::schema_is_official(destination);
}

// Every frame that this constructs spends from the same limit, as how many
// frames it ends up needing is a function of what the resolver hands back
// rather than of the schema the caller passed in. A frame that ran past what
// was left of the limit threw rather than returned, so what it holds is
// always within it
auto charge(std::uint64_t &remaining,
            const sourcemeta::core::SchemaFrame &frame) -> void {
  assert(frame.location_count() <= remaining);
  remaining -= frame.location_count();
}

auto dependencies_internal(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::blaze::DependencyCallback &callback,
    std::string_view default_dialect, std::string_view default_id,
    const sourcemeta::core::SchemaFrame::Paths &paths,
    std::unordered_set<std::string> &visited, std::uint64_t &remaining)
    -> void {
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
      paths,
      "",
      remaining};
  charge(remaining, frame);
  const auto &origin{frame.root()};

  std::vector<
      std::tuple<sourcemeta::core::JSON, sourcemeta::core::JSON::String>>
      found;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) -> void {
    // We don't want to report official schemas, as we can expect
    // virtually all implementations to understand them out of the box
    if (is_skippable_metaschema_reference(pointer, reference.destination)) {
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

    if (!remote.value().is_object() && !remote.value().is_boolean()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    try {
      const sourcemeta::core::SchemaFrame remote_frame{
          sourcemeta::core::SchemaFrame::Mode::Root,
          remote.value(),
          walker,
          resolver,
          default_dialect,
          "",
          sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
          {sourcemeta::core::EMPTY_WEAK_POINTER},
          "",
          remaining};
      charge(remaining, remote_frame);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, sourcemeta::core::to_pointer(pointer),
          "The JSON document is not a valid JSON Schema");
    }

    callback(origin, pointer, identifier, remote.value());
    visited.emplace(identifier);

    // Official schemas can only reference other official schemas, so
    // recursing into them can never surface further dependencies
    if (sourcemeta::core::schema_is_official(identifier)) {
      return;
    }

    found.emplace_back(std::move(remote).value(),
                       sourcemeta::core::JSON::String{identifier});
  });

  for (const auto &entry : found) {
    dependencies_internal(std::get<0>(entry), walker, resolver, callback,
                          default_dialect, std::get<1>(entry),
                          {sourcemeta::core::EMPTY_WEAK_POINTER}, visited,
                          remaining);
  }
}

} // namespace

namespace sourcemeta::blaze {

auto dependencies(const sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaWalker &walker,
                  const sourcemeta::core::SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  std::string_view default_dialect, std::string_view default_id,
                  const sourcemeta::core::SchemaFrame::Paths &paths,
                  const std::uint64_t max_locations) -> void {
  std::unordered_set<std::string> visited;
  auto remaining{max_locations};
  try {
    dependencies_internal(schema, walker, resolver, callback, default_dialect,
                          default_id, paths, visited, remaining);
  } catch (const sourcemeta::core::SchemaFrameLimitError &) {
    // Every frame spends from what is left rather than from the whole, so the
    // one that ran out reports what it was handed. The caller set the limit
    // for the operation, so that is what the operation reports back
    throw sourcemeta::core::SchemaFrameLimitError{max_locations};
  }
}

} // namespace sourcemeta::blaze
