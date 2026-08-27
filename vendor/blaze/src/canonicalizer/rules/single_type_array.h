class SingleTypeArray final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  SingleTypeArray() : SchemaTransformRule{"single_type_array"} {};

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
            {SchemaVocabularies::Known::JSON_Schema_2020_12_Validation,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Validation,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
             SchemaVocabularies::Known::JSON_Schema_Draft_2,
             SchemaVocabularies::Known::JSON_Schema_Draft_1,
             SchemaVocabularies::Known::JSON_Schema_Draft_0}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array() && type->size() == 1 &&
                     type->front().is_string());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto type{schema.at("type").front()};
    schema.at("type").into(std::move(type));
  }
};
