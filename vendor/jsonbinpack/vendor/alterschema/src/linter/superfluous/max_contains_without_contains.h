class MaxContainsWithoutContains final : public Rule {
public:
  MaxContainsWithoutContains()
      : Rule{"max_contains_without_contains",
             "The `maxContains` keyword is meaningless "
             "without the presence of the `contains` keyword"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("maxContains") &&
           !schema.defines("contains");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("maxContains");
  }
};
