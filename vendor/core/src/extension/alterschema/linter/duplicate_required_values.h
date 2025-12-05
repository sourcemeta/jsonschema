class DuplicateRequiredValues final : public SchemaTransformRule {
public:
  DuplicateRequiredValues()
      : SchemaTransformRule{
            "duplicate_required_values",
            "Setting duplicate values in `required` is considered an "
            "anti-pattern"} {};

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
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("required") &&
                     schema.at("required").is_array() &&
                     !schema.at("required").unique());
    // TODO: Highlight which specific entries in `required` are duplicated
    return APPLIES_TO_KEYWORDS("required");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto collection = schema.at("required");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    schema.at("required").into(std::move(collection));
  }
};
