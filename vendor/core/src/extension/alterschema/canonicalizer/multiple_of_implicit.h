class MultipleOfImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MultipleOfImplicit()
      : SchemaTransformRule{"multiple_of_implicit",
                            "The unit of `multipleOf` is the integer 1"} {};

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
                     // Applying this to numbers would be a semantic problem
                     schema.at("type").to_string() == "integer" &&
                     !schema.defines("multipleOf"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("multipleOf", sourcemeta::core::JSON{1});
  }
};
