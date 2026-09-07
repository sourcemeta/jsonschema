class PatternNonEcmaRegex final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  PatternNonEcmaRegex()
      : SchemaTransformRule{
            "pattern_non_ecma_regex",
            "For interoperability reasons, only set this keyword to a regular "
            "expression that strictly adheres to the ECMA-262 dialect"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER}));
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *pattern_value{schema.try_at("pattern")};
    ONLY_CONTINUE_IF(pattern_value && pattern_value->is_string());

    ONLY_CONTINUE_IF(
        !sourcemeta::core::is_regex_ecma(pattern_value->to_string()));
    return applies_to_keywords("pattern");
  }
};
