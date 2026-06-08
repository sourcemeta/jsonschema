class EmptyDependentSchemasDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EmptyDependentSchemasDrop()
      : SchemaTransformRule{"empty_dependent_schemas_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.is_object());

    const auto *dependent_schemas{schema.try_at("dependentSchemas")};
    ONLY_CONTINUE_IF(dependent_schemas && dependent_schemas->is_object() &&
                     dependent_schemas->empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("dependentSchemas");
  }
};
