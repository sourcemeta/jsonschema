class EqualNumericBoundsToEnum final : public Rule {
public:
  EqualNumericBoundsToEnum()
      : Rule{"equal_numeric_bounds_to_enum",
             "Setting `minimum` and `maximum` to the same number only leaves "
             "one possible value"} {};

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
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           (schema.at("type").to_string() == "integer" ||
            schema.at("type").to_string() == "number") &&
           schema.defines("minimum") && schema.at("minimum").is_number() &&
           schema.defines("maximum") && schema.at("maximum").is_number() &&
           schema.at("minimum") == schema.at("maximum");
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    sourcemeta::jsontoolkit::JSON values =
        sourcemeta::jsontoolkit::JSON::make_array();
    values.push_back(transformer.schema().at("minimum"));
    transformer.assign("enum", std::move(values));
    transformer.erase("type");
    transformer.erase("minimum");
    transformer.erase("maximum");
  }
};
