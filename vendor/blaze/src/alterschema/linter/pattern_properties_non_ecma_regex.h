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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
         Vocabularies::Known::JSON_Schema_2019_09_Applicator,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_7_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_6_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_4_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper}));
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
    return APPLIES_TO_POINTERS(std::move(offenders));
  }
};
