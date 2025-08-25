class NonApplicableTypeSpecificKeywords final : public SchemaTransformRule {
public:
  NonApplicableTypeSpecificKeywords()
      : SchemaTransformRule{"non_applicable_type_specific_keywords",
                            "Avoid keywords that don't apply to the type or "
                            "types that the current subschema expects"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());

    std::set<JSON::Type> current_types;
    if (contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-02/hyper-schema#",
                      "http://json-schema.org/draft-01/schema#",
                      "http://json-schema.org/draft-01/hyper-schema#",
                      "http://json-schema.org/draft-00/schema#",
                      "http://json-schema.org/draft-00/hyper-schema#"}) &&
        schema.defines("type")) {
      if (schema.at("type").is_string()) {
        parse_schema_type(
            schema.at("type").to_string(),
            [&current_types](const auto type) { current_types.emplace(type); });
      } else if (schema.at("type").is_array()) {
        for (const auto &entry : schema.at("type").as_array()) {
          if (entry.is_string()) {
            parse_schema_type(entry.to_string(),
                              [&current_types](const auto type) {
                                current_types.emplace(type);
                              });
          }
        }
      }
    }

    if (contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-01/schema#"}) &&
        schema.defines("enum") && schema.at("enum").is_array()) {
      for (const auto &entry : schema.at("enum").as_array()) {
        current_types.emplace(entry.type());
      }
    }

    if (contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#"}) &&
        schema.defines("const")) {
      current_types.emplace(schema.at("const").type());
    }

    // This means that the schema has no explicit type constraints,
    // so we cannot remove anything from it.
    ONLY_CONTINUE_IF(!current_types.empty());

    std::vector<Pointer> positions;
    for (const auto &entry : schema.as_object()) {
      const auto metadata{walker(entry.first, vocabularies)};

      // The keyword applies to any type, so it cannot be removed
      if (metadata.instances.empty()) {
        continue;
      }

      // If none of the types that the keyword applies to is a valid
      // type for the current schema, then by definition we can remove it
      if (std::ranges::none_of(metadata.instances,
                               [&current_types](const auto keyword_type) {
                                 return current_types.contains(keyword_type);
                               })) {
        positions.push_back(Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!positions.empty());
    return APPLIES_TO_POINTERS(std::move(positions));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      schema.erase(location.at(0).to_property());
    }
  }
};
