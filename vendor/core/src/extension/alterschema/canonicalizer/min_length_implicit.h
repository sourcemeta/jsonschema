class MinLengthImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinLengthImplicit()
      : SchemaTransformRule{
            "min_length_implicit",
            "Every string has a minimum length of zero characters"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_1}) &&
                     schema.is_object() && schema.defines("type") &&
                     schema.at("type").is_string() &&
                     schema.at("type").to_string() == "string" &&
                     !schema.defines("minLength"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("minLength", sourcemeta::core::JSON{0});
  }
};
