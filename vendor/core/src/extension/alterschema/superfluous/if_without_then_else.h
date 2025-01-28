class IfWithoutThenElse final : public SchemaTransformRule {
public:
  IfWithoutThenElse()
      : SchemaTransformRule{
            "if_without_then_else",
            "The `if` keyword is meaningless "
            "without the presence of the `then` or `else` keywords"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#"}) &&
           schema.is_object() && schema.defines("if") &&
           !schema.defines("then") && !schema.defines("else");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("if");
  }
};
