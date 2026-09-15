class ConstWithType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ConstWithType()
      : SchemaTransformRule{
            "const_with_type",
            "Setting `type` alongside `const` is considered an anti-pattern, "
            "as the constant already implies its respective type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);
    const auto *const_value{schema.try_at("const")};
    ONLY_CONTINUE_IF(const_value);

    const auto current_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(current_types.any());
    ONLY_CONTINUE_IF(
        current_types.test(std::to_underlying(const_value->type())));
    return applies_to_keywords("const", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
