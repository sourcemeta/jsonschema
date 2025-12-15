class NonApplicableAdditionalItems final : public SchemaTransformRule {
public:
  NonApplicableAdditionalItems()
      : SchemaTransformRule{
            "non_applicable_additional_items",
            "The `additionalItems` keyword is ignored when the "
            "`items` keyword is either not present or set to a schema"} {};

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
                         {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3}) &&
                     schema.is_object() && schema.defines("additionalItems"));

    if (schema.defines("items") && is_schema(schema.at("items"))) {
      return APPLIES_TO_KEYWORDS("additionalItems", "items");
    } else if (!schema.defines("items")) {
      return APPLIES_TO_KEYWORDS("additionalItems");
    } else {
      return false;
    }
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("additionalItems");
  }
};
