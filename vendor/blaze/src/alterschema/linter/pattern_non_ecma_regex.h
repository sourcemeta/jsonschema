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
        {SchemaVocabularies::Known::JSON_Schema_2020_12_Validation,
         SchemaVocabularies::Known::JSON_Schema_2019_09_Validation,
         SchemaVocabularies::Known::JSON_Schema_Draft_7,
         SchemaVocabularies::Known::JSON_Schema_Draft_7_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_6,
         SchemaVocabularies::Known::JSON_Schema_Draft_6_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_4,
         SchemaVocabularies::Known::JSON_Schema_Draft_4_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_3,
         SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_2_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_1,
         SchemaVocabularies::Known::JSON_Schema_Draft_1_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_0,
         SchemaVocabularies::Known::JSON_Schema_Draft_0_Hyper}));
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *pattern_value{schema.try_at("pattern")};
    ONLY_CONTINUE_IF(pattern_value && pattern_value->is_string());

    ONLY_CONTINUE_IF(
        !sourcemeta::core::is_regex_ecma(pattern_value->to_string()));
    return APPLIES_TO_KEYWORDS("pattern");
  }
};
