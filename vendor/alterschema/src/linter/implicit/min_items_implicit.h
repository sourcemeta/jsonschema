class MinItemsImplicit final : public Rule {
public:
  MinItemsImplicit()
      : Rule{"min_items_implicit",
             "Every array has a minimum size of zero items"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(vocabularies,
                        {"http://json-schema.org/draft-07/schema#",
                         "http://json-schema.org/draft-06/schema#",
                         "http://json-schema.org/draft-04/schema#",
                         "http://json-schema.org/draft-03/schema#",
                         "http://json-schema.org/draft-02/hyper-schema#",
                         "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           !schema.defines("minItems");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.assign("minItems", sourcemeta::jsontoolkit::JSON{0});
  }
};
