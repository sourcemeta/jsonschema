class UnsatisfiableMinProperties final : public SchemaTransformRule {
public:
  UnsatisfiableMinProperties()
      : SchemaTransformRule{
            "unsatisfiable_min_properties",
            "Setting `minProperties` to a number less than `required` does "
            "not add any further constraint"} {};

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
           schema.is_object() && schema.defines("minProperties") &&
           schema.at("minProperties").is_integer() &&
           schema.defines("required") && schema.at("required").is_array() &&
           schema.at("required").unique() &&
           schema.at("required").size() >=
               static_cast<std::uint64_t>(
                   schema.at("minProperties").to_integer());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("minProperties");
  }
};
