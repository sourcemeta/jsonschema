class MinPropertiesImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinPropertiesImplicit()
      : SchemaTransformRule{
            "min_properties_implicit",
            "The `minProperties` keyword has a logical default of 0 or the "
            "size of `required`"} {};

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
                     schema.is_object() && schema.defines("type") &&
                     schema.at("type").is_string() &&
                     schema.at("type").to_string() == "object" &&
                     !schema.defines("minProperties"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.defines("required") && schema.at("required").is_array()) {
      schema.assign("minProperties",
                    sourcemeta::core::JSON{schema.at("required").size()});
    } else {
      schema.assign("minProperties", sourcemeta::core::JSON{0});
    }
  }
};
