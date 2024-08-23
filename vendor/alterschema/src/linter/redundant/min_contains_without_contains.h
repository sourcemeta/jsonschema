class MinContainsWithoutContains final : public Rule {
public:
  MinContainsWithoutContains()
      : Rule{"min_contains_without_contains",
             "The `minContains` keyword is meaningless "
             "without the presence of the `contains` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("minContains") &&
           !schema.defines("contains");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("minContains");
  }
};
