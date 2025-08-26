class PropertyNamesTypeDefault final : public SchemaTransformRule {
public:
  PropertyNamesTypeDefault()
      : SchemaTransformRule{
            "property_names_type_default",
            "Setting the `type` keyword to `string` inside "
            "`propertyNames` does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/applicator",
                      "https://json-schema.org/draft/2019-09/vocab/applicator",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#"}) &&
        schema.is_object() && schema.defines("propertyNames") &&
        schema.at("propertyNames").is_object() &&
        schema.at("propertyNames").defines("type") &&
        ((schema.at("propertyNames").at("type").is_string() &&
          schema.at("propertyNames").at("type").to_string() == "string") ||
         (schema.at("propertyNames").at("type").is_array() &&
          std::all_of(schema.at("propertyNames").at("type").as_array().begin(),
                      schema.at("propertyNames").at("type").as_array().end(),
                      [](const auto &item) {
                        return item.is_string() && item.to_string() == "string";
                      }))));
    return APPLIES_TO_POINTERS({{"propertyNames", "type"}});
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("propertyNames").erase("type");
  }
};
