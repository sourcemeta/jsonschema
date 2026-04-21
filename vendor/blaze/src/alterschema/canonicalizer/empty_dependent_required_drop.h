class EmptyDependentRequiredDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EmptyDependentRequiredDrop()
      : SchemaTransformRule{"empty_dependent_required_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object() && schema.defines("dependentRequired") &&
        schema.at("dependentRequired").is_object() &&
        schema.at("dependentRequired").empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("dependentRequired");
  }
};
