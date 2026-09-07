class PatternPropertiesNonEcmaRegex final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  PatternPropertiesNonEcmaRegex()
      : SchemaTransformRule{
            "pattern_properties_non_ecma_regex",
            "For interoperability reasons, only set the keys of this keyword "
            "to regular expressions that strictly adhere to the ECMA-262 "
            "dialect"} {};

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
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}));
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *pattern_properties{schema.try_at("patternProperties")};
    ONLY_CONTINUE_IF(pattern_properties && pattern_properties->is_object() &&
                     !pattern_properties->empty());

    std::vector<Pointer> offenders;
    for (const auto &entry : pattern_properties->as_object()) {
      if (!sourcemeta::core::is_regex_ecma(entry.first)) {
        offenders.push_back(Pointer{"patternProperties", entry.first});
      }
    }

    ONLY_CONTINUE_IF(!offenders.empty());
    return applies_to_pointers(std::move(offenders));
  }
};
