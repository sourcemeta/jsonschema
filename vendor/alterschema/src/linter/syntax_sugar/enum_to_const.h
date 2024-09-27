class EnumToConst final : public Rule {
public:
  EnumToConst()
      : Rule("enum_to_const",
             "An `enum` of a single value can be expressed as `const`") {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
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

  auto transform(Transformer &transformer) const -> void override {
    transformer.assign("const", transformer.schema().at("enum").front());
    transformer.erase("enum");
  }
};
