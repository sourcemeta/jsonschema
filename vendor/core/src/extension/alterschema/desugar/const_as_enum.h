class ConstAsEnum final : public SchemaTransformRule {
public:
  ConstAsEnum()
      : SchemaTransformRule{"const_as_enum",
                            "Setting `const` is syntax sugar for an "
                            "enumeration of a single value"} {};

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
           schema.is_object() && schema.defines("const") &&
           !schema.defines("enum");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    sourcemeta::core::JSON values = sourcemeta::core::JSON::make_array();
    values.push_back(transformer.value().at("const"));
    transformer.assign("enum", values);
    transformer.erase("const");
  }
};
