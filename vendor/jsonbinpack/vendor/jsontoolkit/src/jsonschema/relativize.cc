#include <sourcemeta/jsontoolkit/jsonschema.h>

namespace sourcemeta::jsontoolkit {

auto relativize(JSON &schema, const SchemaWalker &walker,
                const SchemaResolver &resolver,
                const std::optional<std::string> &default_dialect,
                const std::optional<std::string> &default_id) -> void {
  Frame frame;
  frame.analyse(schema, walker, resolver, default_dialect, default_id);

  for (const auto &entry : frame.locations()) {
    if (entry.second.type != Frame::LocationType::Resource &&
        entry.second.type != Frame::LocationType::Subschema) {
      continue;
    }

    auto &subschema{get(schema, entry.second.pointer)};
    assert(is_schema(subschema));
    if (!subschema.is_object()) {
      continue;
    }

    const auto base{URI{entry.second.base}.canonicalize()};
    for (const auto &property : subschema.as_object()) {
      if (walker(property.first, frame.vocabularies(entry.second, resolver))
                  .type != KeywordType::Reference ||
          !property.second.is_string()) {
        continue;
      }

      // In 2019-09, `$recursiveRef` can only be `#`, so there
      // is nothing else we can possibly do
      if (property.first == "$recursiveRef") {
        continue;
      }

      URI reference{property.second.to_string()};
      reference.relative_to(base);
      reference.canonicalize();

      if (reference.is_relative()) {
        subschema.assign(property.first, JSON{reference.recompose()});
      }
    }
  }
}

} // namespace sourcemeta::jsontoolkit
