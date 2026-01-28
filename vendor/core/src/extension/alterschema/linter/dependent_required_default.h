class DependentRequiredDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependentRequiredDefault()
      : SchemaTransformRule{
            "dependent_required_default",
            "Setting the `dependentRequired` keyword to an empty object "
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
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("dependentRequired") &&
        schema.at("dependentRequired").is_object() &&
        schema.at("dependentRequired").empty());
    return APPLIES_TO_KEYWORDS("dependentRequired");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("dependentRequired");
  }
};
