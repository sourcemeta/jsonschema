class MinItemsGivenMinContains final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MinItemsGivenMinContains()
      : SchemaTransformRule{"min_items_given_min_contains"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && !schema.defines("minItems"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() && type->to_string() == "array");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (schema.defines("contains") && schema.defines("minContains") &&
        schema.at("minContains").is_integer()) {
      schema.assign("minItems", sourcemeta::core::JSON{
                                    schema.at("minContains").to_integer()});
    } else {
      schema.assign("minItems", sourcemeta::core::JSON{0});
    }
  }
};
