class DuplicateAllOfBranches final : public SchemaTransformRule {
public:
  DuplicateAllOfBranches()
      : SchemaTransformRule{
            "duplicate_allof_branches",
            "Setting duplicate subschemas in `allOf` is redundant, as it "
            "produces "
            "unnecessary additional validation that is guaranteed to not "
            "affect the validation result"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("allOf") &&
                     schema.at("allOf").is_array() &&
                     !schema.at("allOf").unique());
    // TODO: Highlight which specific entries in `allOf` are duplicated
    return APPLIES_TO_KEYWORDS("allOf");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto collection = schema.at("allOf");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    schema.at("allOf").into(std::move(collection));
  }
};
