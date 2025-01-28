class EnumToConst final : public SchemaTransformRule {
public:
  EnumToConst()
      : SchemaTransformRule{
            "enum_to_const",
            "An `enum` of a single value can be expressed as `const`"} {};

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
           schema.is_object() && !schema.defines("const") &&
           schema.defines("enum") && schema.at("enum").is_array() &&
           schema.at("enum").size() == 1;
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.assign("const", transformer.value().at("enum").front());
    transformer.erase("enum");
  }
};
