class PropertiesImplicit final : public sourcemeta::alterschema::Rule {
public:
  PropertiesImplicit()
      : Rule{"properties_implicit", "Every object has an implicit `properties` "
                                    "that consists of the empty object"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return ((vocabularies.contains(
                 "https://json-schema.org/draft/2020-12/vocab/validation") &&
             vocabularies.contains(
                 "https://json-schema.org/draft/2020-12/vocab/applicator")) ||
            (vocabularies.contains(
                 "https://json-schema.org/draft/2019-09/vocab/validation") &&
             vocabularies.contains(
                 "https://json-schema.org/draft/2019-09/vocab/applicator")) ||
            contains_any(vocabularies,
                         {"http://json-schema.org/draft-07/schema#",
                          "http://json-schema.org/draft-06/schema#",
                          "http://json-schema.org/draft-04/schema#",
                          "http://json-schema.org/draft-03/schema#",
                          "http://json-schema.org/draft-02/hyper-schema#",
                          "http://json-schema.org/draft-01/hyper-schema#"})) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           !schema.defines("properties");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.assign("properties",
                       sourcemeta::jsontoolkit::JSON::make_object());
  }
};
