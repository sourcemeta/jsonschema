class IntegerBoundedMultiplier8Bit final
    : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerBoundedMultiplier8Bit()
      : sourcemeta::core::SchemaTransformRule{
            "integer_bounded_multiplier_8_bit", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (location.dialect != "https://json-schema.org/draft/2020-12/schema" ||
        !vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/validation") ||
        !schema.defines("type") || schema.at("type").to_string() != "integer" ||
        !schema.defines("minimum") || !schema.at("minimum").is_integer() ||
        !schema.defines("maximum") || !schema.at("maximum").is_integer() ||
        !schema.defines("multipleOf") ||
        !schema.at("multipleOf").is_integer()) {
      return false;
    }

    return is_byte(count_multiples(schema.at("minimum").to_integer(),
                                   schema.at("maximum").to_integer(),
                                   schema.at("multipleOf").to_integer()));
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto minimum = schema.at("minimum");
    auto maximum = schema.at("maximum");
    auto multiplier = schema.at("multipleOf");

    auto options = sourcemeta::core::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", std::move(multiplier));
    make_encoding(schema, "BOUNDED_MULTIPLE_8BITS_ENUM_FIXED", options);
  }
};
