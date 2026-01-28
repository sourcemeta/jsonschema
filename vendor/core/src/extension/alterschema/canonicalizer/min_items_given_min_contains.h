class MinItemsGivenMinContains final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinItemsGivenMinContains()
      : SchemaTransformRule{
            "min_items_given_min_contains",
            "Every array has a minimum size of zero items but may be affected "
            "by `minContains`"} {};

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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "array" &&
        !schema.defines("minItems"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.defines("contains") && schema.defines("minContains") &&
        schema.at("minContains").is_integer()) {
      schema.assign("minItems", sourcemeta::core::JSON{
                                    schema.at("minContains").to_integer()});
    } else {
      schema.assign("minItems", sourcemeta::core::JSON{0});
    }
  }
};
