class IntegerUnboundMultiplier final
    : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerUnboundMultiplier()
      : sourcemeta::core::SchemaTransformRule{"integer_unbound_multiplier",
                                              ""} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") &&
           schema.at("type").to_string() == "integer" &&
           !schema.defines("minimum") && !schema.defines("maximum") &&
           schema.defines("multipleOf") && schema.at("multipleOf").is_integer();
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto multiplier = transformer.value().at("multipleOf");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("multiplier", std::move(multiplier));
    make_encoding(transformer, "ARBITRARY_MULTIPLE_ZIGZAG_VARINT", options);
  }
};
