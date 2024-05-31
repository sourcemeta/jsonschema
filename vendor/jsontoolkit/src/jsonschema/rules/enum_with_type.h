class EnumWithType final : public SchemaTransformRule {
public:
  EnumWithType()
      : SchemaTransformRule{
            "enum_with_type",
            "Setting `type` alongside `enum` is considered an anti-pattern, as "
            "the enumeration choices already imply their respective types"} {};

  [[nodiscard]] auto condition(const JSON &schema, const std::string &,
                               const std::set<std::string> &vocabularies,
                               const Pointer &) const -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}),
           schema.is_object() && schema.defines("type") &&
               schema.defines("enum");
  }

  auto transform(SchemaTransformer &transformer) const -> void override {
    transformer.erase("type");
  }
};
