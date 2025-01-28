class DuplicateRequiredValues final : public SchemaTransformRule {
public:
  DuplicateRequiredValues()
      : SchemaTransformRule{
            "duplicate_required_values",
            "Setting duplicate values in `required` is considered an "
            "anti-pattern"} {};

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
           schema.is_object() && schema.defines("required") &&
           schema.at("required").is_array() && !schema.at("required").unique();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    auto collection = transformer.value().at("required");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    transformer.replace({"required"}, std::move(collection));
  }
};
