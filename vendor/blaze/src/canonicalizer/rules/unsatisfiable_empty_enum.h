class UnsatisfiableEmptyEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  UnsatisfiableEmptyEnum() : SchemaTransformRule{"unsatisfiable_empty_enum"} {};

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
            {SchemaVocabularies::Known::JSON_Schema_Draft_0,
             SchemaVocabularies::Known::JSON_Schema_Draft_1,
             SchemaVocabularies::Known::JSON_Schema_Draft_2,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Validation,
             SchemaVocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object());

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     enum_value->empty());
    this->unsatisfiable_ = UNSATISFIABLE_SCHEMA(vocabularies);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    INTO_UNSATISFIABLE(schema, this->unsatisfiable_);
  }

private:
  mutable sourcemeta::core::JSON unsatisfiable_{false};
};
