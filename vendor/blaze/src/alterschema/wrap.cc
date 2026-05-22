#include <sourcemeta/blaze/alterschema.h>

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>   // std::any_of
#include <cassert>     // assert
#include <functional>  // std::cref
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

auto wrap(const sourcemeta::core::JSON &schema, const SchemaFrame &frame,
          const SchemaFrame::Location &location, const SchemaResolver &resolver,
          sourcemeta::core::WeakPointer &base) -> sourcemeta::core::JSON {
  assert(frame.mode() == SchemaFrame::Mode::References);
  assert(location.type != SchemaFrame::LocationType::Pointer);

  const auto &pointer{location.pointer};
  if (pointer.empty()) {
    auto copy = schema;
    if (copy.is_object()) {
      copy.assign("$schema", sourcemeta::core::JSON{location.dialect});
    }

    return copy;
  }

  assert(sourcemeta::core::try_get(schema, pointer));
  const auto has_internal_references{
      std::any_of(frame.references().cbegin(), frame.references().cend(),
                  [&pointer](const auto &reference) {
                    return reference.first.second.starts_with(pointer);
                  })};

  if (!has_internal_references) {
    auto subschema{sourcemeta::core::get(schema, pointer)};
    if (subschema.is_object()) {
      subschema.assign("$schema", sourcemeta::core::JSON{location.dialect});
    }

    return subschema;
  }

  auto copy = schema;
  copy.assign("$schema", sourcemeta::core::JSON{location.dialect});

  auto result{sourcemeta::core::JSON::make_object()};
  // JSON Schema 2020-12 is the first dialect that truly supports
  // cross-dialect references In practice, others do, but we can
  // play it safe here
  result.assign_assume_new(
      "$schema",
      sourcemeta::core::JSON{"https://json-schema.org/draft/2020-12/schema"});
  // We need to make sure the schema we are wrapping always has an identifier,
  // at least an artificial one, otherwise a standalone instance of `$schema`
  // outside of the root of a schema resource is not valid according to
  // JSON Schema
  // However, note that we use a relative URI so that references to
  // other schemas whose top-level identifiers are relative URIs don't
  // get affected. Otherwise, we would cause unintended base resolution.
  constexpr std::string_view WRAPPER_IDENTIFIER{"__sourcemeta-core-wrap__"};
  const auto maybe_id{identify(copy, resolver, location.dialect)};
  const auto id{maybe_id.empty() ? WRAPPER_IDENTIFIER : maybe_id};

  sourcemeta::core::URI uri{id};

  try {
    reidentify(copy, id, resolver, location.dialect);

    // Otherwise we will get an error with the `WRAPPER_IDENTIFIER`, which will
    // be confusing to end users
  } catch (const SchemaReferenceObjectResourceError &) {
    throw SchemaError(
        "Cannot process a JSON Schema Draft 7 or older with a top-level "
        "`$ref` (which overrides sibling keywords) without introducing "
        "undefined behavior");
  }

  result.assign_assume_new("$defs", sourcemeta::core::JSON::make_object());
  result.at("$defs").assign_assume_new("schema", std::move(copy));

  // Add a reference to the schema
  if (!uri.fragment().has_value() || uri.fragment().value().empty()) {
    uri.fragment(sourcemeta::core::to_string(pointer));
    result.assign_assume_new("$ref", sourcemeta::core::JSON{uri.recompose()});
  } else {
    static const sourcemeta::core::JSON::String DEFS{"$defs"};
    static const sourcemeta::core::JSON::String SCHEMA{"schema"};
    result.assign_assume_new(
        "$ref",
        sourcemeta::core::JSON{
            sourcemeta::core::to_uri(sourcemeta::core::WeakPointer{
                                         std::cref(DEFS), std::cref(SCHEMA)}
                                         .concat(pointer))
                .recompose()});
  }

  static const sourcemeta::core::JSON::String REF{"$ref"};
  base.push_back(REF);
  return result;
}

} // namespace sourcemeta::blaze
