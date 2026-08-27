class EmptyDependenciesDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EmptyDependenciesDrop() : SchemaTransformRule{"empty_dependencies_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_Schema_Draft_3,
                          SchemaVocabularies::Known::JSON_Schema_Draft_4,
                          SchemaVocabularies::Known::JSON_Schema_Draft_6,
                          SchemaVocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object());

    const auto *dependencies{schema.try_at("dependencies")};
    ONLY_CONTINUE_IF(dependencies && dependencies->is_object() &&
                     dependencies->empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("dependencies");
  }
};
