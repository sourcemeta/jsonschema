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

  auto transform(JSON &schema) const -> void override {
    auto values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("const"));
    schema.at("const").into(std::move(values));
    schema.rename("const", "enum");
  }
};
