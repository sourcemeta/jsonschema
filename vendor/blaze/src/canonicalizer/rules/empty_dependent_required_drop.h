class EmptyDependentRequiredDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EmptyDependentRequiredDrop()
      : SchemaTransformRule{"empty_dependent_required_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object());

    const auto *dependent_required{schema.try_at("dependentRequired")};
    ONLY_CONTINUE_IF(dependent_required && dependent_required->is_object() &&
                     dependent_required->empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("dependentRequired");
  }
};
