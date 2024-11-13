class IntegerBounded8Bit final : public sourcemeta::alterschema::Rule {
 public:
  IntegerBounded8Bit()
      : sourcemeta::alterschema::Rule{"integer_bounded_8_bit", ""} {};

  [[nodiscard]] auto condition(
      const sourcemeta::jsontoolkit::JSON &schema, const std::string &dialect,
      const std::set<std::string> &vocabularies,
      const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
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

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    auto minimum = transformer.schema().at("minimum");
    auto maximum = transformer.schema().at("maximum");
    auto options = sourcemeta::jsontoolkit::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", sourcemeta::jsontoolkit::JSON{1});
    make_encoding(transformer, "BOUNDED_MULTIPLE_8BITS_ENUM_FIXED", options);
  }
};
