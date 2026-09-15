class UnsatisfiableTypeAndEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  UnsatisfiableTypeAndEnum()
      : SchemaTransformRule{"unsatisfiable_type_and_enum"} {};

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
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string());
    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     !enum_value->empty());

    const auto declared_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(declared_types.any());
    const bool integer_matches_integral{
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION}) &&
        declared_types.test(
            std::to_underlying(sourcemeta::core::JSON::Type::Integer))};
    ONLY_CONTINUE_IF(std::ranges::none_of(
        enum_value->as_array(),
        [&declared_types, integer_matches_integral](const auto &value) -> auto {
          return declared_types.test(std::to_underlying(value.type())) ||
                 (integer_matches_integral && value.is_integral());
        }));
    this->unsatisfiable_ = unsatisfiable_schema(vocabularies);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    into_unsatisfiable(schema, this->unsatisfiable_);
  }

private:
  mutable sourcemeta::core::JSON unsatisfiable_{false};
};
