class EqualNumericBoundsToEnum final : public SchemaTransformRule {
public:
  EqualNumericBoundsToEnum()
      : SchemaTransformRule{
            "equal_numeric_bounds_to_enum",
            "Setting `minimum` and `maximum` to the same number only leaves "
            "one possible value"} {};

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
                     {"http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-01/schema#"}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        (schema.at("type").to_string() == "integer" ||
         schema.at("type").to_string() == "number") &&
        schema.defines("minimum") && schema.at("minimum").is_number() &&
        schema.defines("maximum") && schema.at("maximum").is_number() &&
        schema.at("minimum") == schema.at("maximum"));
    return APPLIES_TO_KEYWORDS("minimum", "maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    sourcemeta::core::JSON values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("minimum"));
    schema.assign("enum", std::move(values));
    schema.erase("type");
    schema.erase("minimum");
    schema.erase("maximum");
  }
};
