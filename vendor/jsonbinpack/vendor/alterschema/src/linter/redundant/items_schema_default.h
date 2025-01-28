class ItemsSchemaDefault final : public Rule {
public:
  ItemsSchemaDefault()
      : Rule{"items_schema_default",
             "Setting the `items` keyword to the true schema "
             "does not add any further constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("items") &&
           ((schema.at("items").is_boolean() &&
             schema.at("items").to_boolean()) ||
            (schema.at("items").is_object() && schema.at("items").empty()));
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("items");
  }
};
