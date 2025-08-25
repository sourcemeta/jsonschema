class EnumWithType final : public SchemaTransformRule {
public:
  EnumWithType()
      : SchemaTransformRule{
            "enum_with_type",
            "Setting `type` alongside `enum` is considered an anti-pattern, as "
            "the enumeration choices already imply their respective types"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(contains_any(
        vocabularies, {"https://json-schema.org/draft/2020-12/vocab/validation",
                       "https://json-schema.org/draft/2019-09/vocab/validation",
                       "http://json-schema.org/draft-07/schema#",
                       "http://json-schema.org/draft-06/schema#",
                       "http://json-schema.org/draft-04/schema#",
                       "http://json-schema.org/draft-03/schema#",
                       "http://json-schema.org/draft-02/schema#",
                       "http://json-schema.org/draft-01/schema#"}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("type") &&
                     schema.defines("enum") && schema.at("enum").is_array());

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

    ONLY_CONTINUE_IF(std::ranges::all_of(
        schema.at("enum").as_array(), [&current_types](const auto &item) {
          return current_types.contains(item.type());
        }));

    return APPLIES_TO_KEYWORDS("enum", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
