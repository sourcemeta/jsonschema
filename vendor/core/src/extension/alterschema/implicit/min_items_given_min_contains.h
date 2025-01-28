class MinItemsGivenMinContains final : public SchemaTransformRule {
public:
  MinItemsGivenMinContains()
      : SchemaTransformRule{
            "min_items_given_min_contains",
            "Every array has a minimum size of zero items but may be affected "
            "by `minContains`"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           !schema.defines("minItems");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    if (transformer.value().defines("contains") &&
        transformer.value().defines("minContains") &&
        transformer.value().at("minContains").is_integer()) {
      transformer.assign(
          "minItems", sourcemeta::core::JSON{
                          transformer.value().at("minContains").to_integer()});
    } else {
      transformer.assign("minItems", sourcemeta::core::JSON{0});
    }
  }
};
