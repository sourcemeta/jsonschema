class ConstWithType final : public SchemaTransformRule {
public:
  ConstWithType()
      : SchemaTransformRule{
            "const_with_type",
            "Setting `type` alongside `const` is considered an anti-pattern, "
            "as the constant already implies its respective type"} {};

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
                "http://json-schema.org/draft-06/schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.defines("const");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("type");
  }
};
