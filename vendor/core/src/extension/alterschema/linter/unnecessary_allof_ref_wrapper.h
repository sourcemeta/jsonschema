class UnnecessaryAllOfRefWrapper final : public SchemaTransformRule {
public:
  UnnecessaryAllOfRefWrapper()
      : SchemaTransformRule{"unnecessary_allof_ref_wrapper",
                            "Wrapping `$ref` in `allOf` is unnecessary in JSON "
                            "Schema 2019-09 and later versions as `$ref` does "
                            "not override sibling keywords anymore"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (!((vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/core") &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/applicator")) ||
          (vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/core") &&
           vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/applicator")))) {
      return false;
    }

    if (!schema.is_object() || schema.defines("$ref") ||
        !schema.defines("allOf") || !schema.at("allOf").is_array() ||
        schema.at("allOf").empty()) {
      return false;
    }

    bool match{false};
    for (const auto &entry : schema.at("allOf").as_array()) {
      if (entry.is_object() && entry.defines("$ref")) {
        if (match) {
          return false;
        } else {
          match = true;
        }
      }
    }

    return match;
  }

  auto transform(JSON &schema) const -> void override {
    // TODO: We should have a way for the condition to pass data to the
    // transform, so we don't have to loop from scratch once more to figure out
    // what index to remove
    auto &array{schema.at("allOf").as_array()};
    auto iterator{array.begin()};
    for (; iterator != array.end(); ++iterator) {
      if (iterator->is_object() && iterator->defines("$ref")) {
        break;
      }
    }

    assert(iterator != array.end());
    auto reference{std::move(iterator->at("$ref"))};
    if (iterator->size() == 1) {
      schema.at("allOf").erase(iterator);
      if (schema.at("allOf").empty()) {
        schema.erase("allOf");
      }
    } else {
      iterator->erase("$ref");
    }

    schema.assign("$ref", std::move(reference));
  }
};
