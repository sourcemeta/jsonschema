class RequiredPropertiesInProperties final : public SchemaTransformRule {
public:
  RequiredPropertiesInProperties()
      : SchemaTransformRule{
            "strict/required_properties_in_properties",
            "Every property listed in the `required` keyword must be "
            "explicitly defined using the `properties` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return ((vocabularies.contains(
                 "https://json-schema.org/draft/2020-12/vocab/validation") &&
             vocabularies.contains(
                 "https://json-schema.org/draft/2020-12/vocab/applicator")) ||
            (vocabularies.contains(
                 "https://json-schema.org/draft/2019-09/vocab/validation") &&
             vocabularies.contains(
                 "https://json-schema.org/draft/2019-09/vocab/applicator")) ||
            contains_any(vocabularies,
                         {"http://json-schema.org/draft-07/schema#",
                          "http://json-schema.org/draft-06/schema#",
                          "http://json-schema.org/draft-04/schema#",
                          "http://json-schema.org/draft-03/schema#"})) &&
           schema.is_object() && schema.defines("required") &&
           schema.at("required").is_array() && !schema.at("required").empty() &&
           std::ranges::any_of(schema.at("required").as_array(),
                               [&schema](const auto &property) {
                                 return property.is_string() &&
                                        (!schema.defines("properties") ||
                                         (schema.at("properties").is_object() &&
                                          !schema.at("properties")
                                               .defines(property.to_string())));
                               });
  }

  auto transform(JSON &schema) const -> void override {
    schema.assign_if_missing("properties",
                             sourcemeta::core::JSON::make_object());
    for (const auto &property : schema.at("required").as_array()) {
      if (property.is_string() &&
          !schema.at("properties").defines(property.to_string())) {
        schema.at("properties")
            .assign(property.to_string(), sourcemeta::core::JSON{true});
      }
    }
  }
};
