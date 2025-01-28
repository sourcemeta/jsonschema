#include <sourcemeta/core/jsonschema.h>

namespace sourcemeta::core {

auto unidentify(JSON &schema, const SchemaWalker &walker,
                const SchemaResolver &resolver,
                const std::optional<std::string> &default_dialect) -> void {
  // (1) Re-frame before changing anything
  Frame frame;
  frame.analyse(schema, walker, resolver, default_dialect);

  // (2) Remove all identifiers and anchors
  for (const auto &entry :
       SchemaIterator{schema, walker, resolver, default_dialect}) {
    auto &subschema{get(schema, entry.pointer)};
    if (subschema.is_boolean()) {
      continue;
    }

    assert(entry.base_dialect.has_value());
    anonymize(subschema, entry.base_dialect.value());

    if (entry.vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/core")) {
      subschema.erase("$anchor");
      subschema.erase("$dynamicAnchor");
    }

    if (entry.vocabularies.contains(
            "https://json-schema.org/draft/2019-09/vocab/core")) {
      subschema.erase("$anchor");
      subschema.erase("$recursiveAnchor");
    }
  }

  // (3) Fix-up reference based on pointers from the root
  for (const auto &[key, reference] : frame.references()) {
    const auto result{frame.traverse(reference.destination)};
    if (result.has_value()) {
      set(schema, key.second,
          JSON{to_uri(result.value().get().pointer).recompose()});
    } else if (!key.second.empty() && key.second.back().is_property() &&
               key.second.back().to_property() != "$schema") {
      set(schema, key.second, JSON{reference.destination});
    }
  }
}

} // namespace sourcemeta::core
