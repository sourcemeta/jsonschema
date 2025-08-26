class MinPropertiesCoveredByRequired final : public SchemaTransformRule {
public:
  MinPropertiesCoveredByRequired()
      : SchemaTransformRule{
            "min_properties_covered_by_required",
            "Setting `minProperties` to a number less than `required` does "
            "not add any further constraint"} {};

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
                      "http://json-schema.org/draft-04/schema#"}) &&
        schema.is_object() && schema.defines("minProperties") &&
        schema.at("minProperties").is_integer() && schema.defines("required") &&
        schema.at("required").is_array() && schema.at("required").unique() &&
        std::cmp_greater(schema.at("required").size(),
                         static_cast<std::uint64_t>(
                             schema.at("minProperties").to_integer())));
    return APPLIES_TO_KEYWORDS("minProperties", "required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("minProperties",
                  sourcemeta::core::JSON{schema.at("required").size()});
  }
};
