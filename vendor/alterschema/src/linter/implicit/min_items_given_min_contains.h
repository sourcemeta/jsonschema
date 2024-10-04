class MinItemsGivenMinContains final : public sourcemeta::alterschema::Rule {
public:
  MinItemsGivenMinContains()
      : Rule{"min_items_given_min_contains",
             "Every array has a minimum size of zero items but may be affected "
             "by `minContains`"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
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

  auto transform(Transformer &transformer) const -> void override {
    if (transformer.schema().defines("contains") &&
        transformer.schema().defines("minContains") &&
        transformer.schema().at("minContains").is_integer()) {
      transformer.assign(
          "minItems", sourcemeta::jsontoolkit::JSON{
                          transformer.schema().at("minContains").to_integer()});
    } else {
      transformer.assign("minItems", sourcemeta::jsontoolkit::JSON{0});
    }
  }
};
