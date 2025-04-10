class MaxContainsCoveredByMaxItems final : public SchemaTransformRule {
public:
  MaxContainsCoveredByMaxItems()
      : SchemaTransformRule{
            "max_contains_covered_by_max_items",
            "Setting the `maxContains` keyword to a number greater than or "
            "equal to the array upper bound does not add any further "
            "constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
           schema.is_object() && schema.defines("maxContains") &&
           schema.at("maxContains").is_integer() &&
           schema.defines("maxItems") && schema.at("maxItems").is_integer() &&
           schema.at("maxContains").to_integer() >
               schema.at("maxItems").to_integer();
  }

  auto transform(JSON &schema) const -> void override {
    schema.assign("maxContains", schema.at("maxItems"));
  }
};
