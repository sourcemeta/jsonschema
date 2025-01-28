class DuplicateEnumValues final : public SchemaTransformRule {
public:
  DuplicateEnumValues()
      : SchemaTransformRule{"duplicate_enum_values",
                            "Setting duplicate values in `enum` is "
                            "considered an anti-pattern"} {};

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
           schema.is_object() && schema.defines("enum") &&
           schema.at("enum").is_array() && !schema.at("enum").unique();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    auto collection = transformer.value().at("enum");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    transformer.replace({"enum"}, std::move(collection));
  }
};
