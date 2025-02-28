class IntegerBounded8Bit final : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerBounded8Bit()
      : sourcemeta::core::SchemaTransformRule{"integer_bounded_8_bit", ""} {};

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
           schema.defines("minimum") && schema.defines("maximum") &&
           is_byte(schema.at("maximum").to_integer() -
                   schema.at("minimum").to_integer()) &&
           !schema.defines("multipleOf");
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto minimum = transformer.value().at("minimum");
    auto maximum = transformer.value().at("maximum");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", sourcemeta::core::JSON{1});
    make_encoding(transformer, "BOUNDED_MULTIPLE_8BITS_ENUM_FIXED", options);
  }
};
