class DeprecatedFalseDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DeprecatedFalseDrop() : SchemaTransformRule{"deprecated_false_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_META_DATA,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_META_DATA}) &&
        schema.is_object());

    const auto *deprecated{schema.try_at("deprecated")};
    ONLY_CONTINUE_IF(deprecated && deprecated->is_boolean() &&
                     !deprecated->to_boolean());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("deprecated");
  }
};
