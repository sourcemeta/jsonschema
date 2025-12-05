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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
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
