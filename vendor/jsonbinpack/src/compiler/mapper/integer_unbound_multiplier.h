class IntegerUnboundMultiplier final : public sourcemeta::alterschema::Rule {
public:
  IntegerUnboundMultiplier()
      : sourcemeta::alterschema::Rule{"integer_unbound_multiplier", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") &&
           schema.at("type").to_string() == "integer" &&
           !schema.defines("minimum") && !schema.defines("maximum") &&
           schema.defines("multipleOf") && schema.at("multipleOf").is_integer();
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    auto multiplier = transformer.schema().at("multipleOf");
    auto options = sourcemeta::jsontoolkit::JSON::make_object();
    options.assign("multiplier", std::move(multiplier));
    make_encoding(transformer, "ARBITRARY_MULTIPLE_ZIGZAG_VARINT", options);
  }
};
