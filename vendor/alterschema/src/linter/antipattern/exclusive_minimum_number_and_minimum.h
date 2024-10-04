class ExclusiveMinimumNumberAndMinimum final : public Rule {
public:
  ExclusiveMinimumNumberAndMinimum()
      : Rule{"exclusive_minimum_number_and_minimum",
             "Setting both `exclusiveMinimum` and `minimum` at the same time "
             "is considered an anti-pattern. You should choose one"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#"}) &&
           schema.is_object() && schema.defines("minimum") &&
           schema.defines("exclusiveMinimum") &&
           schema.at("minimum").is_number() &&
           schema.at("exclusiveMinimum").is_number();
  }

  auto transform(Transformer &transformer) const -> void override {
    if (transformer.schema().at("exclusiveMinimum") <
        transformer.schema().at("minimum")) {
      transformer.erase("exclusiveMinimum");
    } else {
      transformer.erase("minimum");
    }
  }
};
