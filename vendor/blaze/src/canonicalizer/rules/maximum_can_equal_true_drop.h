class MaximumCanEqualTrueDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MaximumCanEqualTrueDrop()
      : SchemaTransformRule{"maximum_can_equal_true_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));
    const auto *maximum_can_equal{schema.try_at("maximumCanEqual")};
    ONLY_CONTINUE_IF(maximum_can_equal && maximum_can_equal->is_boolean() &&
                     maximum_can_equal->to_boolean());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("maximumCanEqual");
  }
};
