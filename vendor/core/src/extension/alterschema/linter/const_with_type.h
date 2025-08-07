class ConstWithType final : public SchemaTransformRule {
public:
  ConstWithType()
      : SchemaTransformRule{
            "const_with_type",
            "Setting `type` alongside `const` is considered an anti-pattern, "
            "as the constant already implies its respective type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (!contains_any(vocabularies,
                      {"https://json-schema.org/draft/2020-12/vocab/validation",
                       "https://json-schema.org/draft/2019-09/vocab/validation",
                       "http://json-schema.org/draft-07/schema#",
                       "http://json-schema.org/draft-06/schema#"})) {
      return false;
    }

    if (!schema.is_object() || !schema.defines("type") ||
        !schema.defines("const")) {
      return false;
    }

    std::set<JSON::Type> current_types;
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

    return current_types.contains(schema.at("const").type());
  }

  auto transform(JSON &schema) const -> void override { schema.erase("type"); }
};
