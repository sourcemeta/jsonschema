class MinLengthImplicit final : public SchemaTransformRule {
public:
  MinLengthImplicit()
      : SchemaTransformRule{
            "min_length_implicit",
            "Every string has a minimum length of zero characters"} {};

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
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-01/schema#"}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "string" &&
        !schema.defines("minLength"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("minLength", sourcemeta::core::JSON{0});
  }
};
