class UnevaluatedPropertiesDefault final : public SchemaTransformRule {
public:
  UnevaluatedPropertiesDefault()
      : SchemaTransformRule{
            "unevaluated_properties_default",
            "Setting the `unevaluatedProperties` keyword to the true schema "
            "does not add any further constraint"} {};

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
        contains_any(
            vocabularies,
            {"https://json-schema.org/draft/2020-12/vocab/unevaluated",
             "https://json-schema.org/draft/2019-09/vocab/applicator"}) &&
        schema.is_object() && schema.defines("unevaluatedProperties") &&
        ((schema.at("unevaluatedProperties").is_boolean() &&
          schema.at("unevaluatedProperties").to_boolean()) ||
         (schema.at("unevaluatedProperties").is_object() &&
          schema.at("unevaluatedProperties").empty())));
    return APPLIES_TO_KEYWORDS("unevaluatedProperties");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("unevaluatedProperties");
  }
};
