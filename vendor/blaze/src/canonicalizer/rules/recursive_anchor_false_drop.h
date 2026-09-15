class RecursiveAnchorFalseDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  RecursiveAnchorFalseDrop()
      : SchemaTransformRule{"recursive_anchor_false_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains(
                         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE) &&
                     schema.is_object());

    const auto *recursive_anchor{schema.try_at("$recursiveAnchor")};
    ONLY_CONTINUE_IF(recursive_anchor && recursive_anchor->is_boolean() &&
                     !recursive_anchor->to_boolean());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("$recursiveAnchor");
  }
};
