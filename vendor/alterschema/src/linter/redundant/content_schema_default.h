class ContentSchemaDefault final : public Rule {
public:
  ContentSchemaDefault()
      : Rule{"content_schema_default",
             "Setting the `contentSchema` keyword to the true schema "
             "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/content",
                "https://json-schema.org/draft/2019-09/vocab/content"}) &&
           schema.is_object() && schema.defines("contentSchema") &&
           ((schema.at("contentSchema").is_boolean() &&
             schema.at("contentSchema").to_boolean()) ||
            (schema.at("contentSchema").is_object() &&
             schema.at("contentSchema").empty()));
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("contentSchema");
  }
};
