class ItemsImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ItemsImplicit()
      : SchemaTransformRule{"items_implicit",
                            "Every array has an implicit `items` "
                            "that consists of the boolean schema `true`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        ((vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Validation) &&
          vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Applicator)) ||
         (vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Validation) &&
          vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Applicator)) ||
         vocabularies.contains_any(
             {Vocabularies::Known::JSON_Schema_Draft_7,
              Vocabularies::Known::JSON_Schema_Draft_6})) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "array" && !schema.defines("items"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("items", JSON{true});
  }
};
