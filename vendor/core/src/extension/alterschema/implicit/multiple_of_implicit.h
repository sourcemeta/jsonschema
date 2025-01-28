class MultipleOfImplicit final : public SchemaTransformRule {
public:
  MultipleOfImplicit()
      : SchemaTransformRule{"multiple_of_implicit",
                            "The unit of `multipleOf` is the integer 1"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           (schema.at("type").to_string() == "integer" ||
            schema.at("type").to_string() == "number") &&
           !schema.defines("multipleOf");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.assign("multipleOf", sourcemeta::core::JSON{1});
  }
};
