class MinPropertiesImplicit final : public SchemaTransformRule {
public:
  MinPropertiesImplicit()
      : SchemaTransformRule{
            "min_properties_implicit",
            "The `minProperties` keyword has a logical default of 0 or the "
            "size of `required`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           !schema.defines("minProperties");
  }

  auto transform(JSON &schema) const -> void override {
    if (schema.defines("required") && schema.at("required").is_array()) {
      schema.assign("minProperties",
                    sourcemeta::core::JSON{schema.at("required").size()});
    } else {
      schema.assign("minProperties", sourcemeta::core::JSON{0});
    }
  }
};
