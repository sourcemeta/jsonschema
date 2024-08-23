class UnevaluatedPropertiesDefault final : public Rule {
public:
  UnevaluatedPropertiesDefault()
      : Rule{"unevaluated_properties_default",
             "Setting the `unevaluatedProperties` keyword to the true schema "
             "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/unevaluated",
                "https://json-schema.org/draft/2019-09/vocab/applicator"}) &&
           schema.is_object() && schema.defines("unevaluatedProperties") &&
           ((schema.at("unevaluatedProperties").is_boolean() &&
             schema.at("unevaluatedProperties").to_boolean()) ||
            (schema.at("unevaluatedProperties").is_object() &&
             schema.at("unevaluatedProperties").empty()));
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("unevaluatedProperties");
  }
};
