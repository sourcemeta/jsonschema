class DuplicateAnyOfBranches final : public SchemaTransformRule {
public:
  DuplicateAnyOfBranches()
      : SchemaTransformRule{
            "duplicate_anyof_branches",
            "Setting duplicate subschemas in `anyOf` is redundant, as it "
            "produces "
            "unnecessary additional validation that is guaranteed to not "
            "affect the validation result"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#"}) &&
           schema.is_object() && schema.defines("anyOf") &&
           schema.at("anyOf").is_array() && !schema.at("anyOf").unique();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    auto collection = transformer.value().at("anyOf");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    transformer.replace({"anyOf"}, std::move(collection));
  }
};
