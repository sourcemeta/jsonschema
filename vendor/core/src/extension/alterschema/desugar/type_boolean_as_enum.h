class TypeBooleanAsEnum final : public SchemaTransformRule {
public:
  TypeBooleanAsEnum()
      : SchemaTransformRule{
            "type_boolean_as_enum",
            "Setting `type` to `boolean` is syntax sugar for an enumeration "
            "of two values: `false` and `true`"} {};

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
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "boolean" &&
           !schema.defines("enum") && !schema.defines("const");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    auto choices = sourcemeta::core::JSON::make_array();
    choices.push_back(sourcemeta::core::JSON{false});
    choices.push_back(sourcemeta::core::JSON{true});
    transformer.assign("enum", choices);
    transformer.erase("type");
  }
};
