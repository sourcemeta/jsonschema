class IntegerBounded8Bit final : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerBounded8Bit()
      : sourcemeta::core::SchemaTransformRule{"integer_bounded_8_bit", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return location.dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") &&
           schema.at("type").to_string() == "integer" &&
           schema.defines("minimum") && schema.defines("maximum") &&
           is_byte(schema.at("maximum").to_integer() -
                   schema.at("minimum").to_integer()) &&
           !schema.defines("multipleOf");
  }

  auto transform(sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaTransformRule::Result &) const
      -> void override {
    auto minimum = schema.at("minimum");
    auto maximum = schema.at("maximum");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", sourcemeta::core::JSON{1});
    make_encoding(schema, "BOUNDED_MULTIPLE_8BITS_ENUM_FIXED", options);
  }
};
