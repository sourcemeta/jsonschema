class TypeNullAsEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  TypeNullAsEnum() : SchemaTransformRule{"type_null_as_enum"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0}) &&
        schema.is_object() && !schema.defines("enum") &&
        !schema.defines("const"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "null");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto choices = sourcemeta::core::JSON::make_array();
    choices.push_back(sourcemeta::core::JSON{nullptr});
    schema.at("type").into(std::move(choices));
    schema.rename("type", "enum");
  }
};
