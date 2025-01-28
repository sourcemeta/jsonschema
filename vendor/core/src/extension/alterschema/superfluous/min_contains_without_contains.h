class MinContainsWithoutContains final : public SchemaTransformRule {
public:
  MinContainsWithoutContains()
      : SchemaTransformRule{"min_contains_without_contains",
                            "The `minContains` keyword is meaningless "
                            "without the presence of the `contains` keyword"} {
        };

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("minContains") &&
           !schema.defines("contains");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("minContains");
  }
};
