class IntegerUpperBoundMultiplier final
    : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerUpperBoundMultiplier()
      : sourcemeta::core::SchemaTransformRule{"integer_upper_bound_multiplier",
                                              ""} {};

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
           !schema.defines("minimum") && schema.defines("maximum") &&
           schema.defines("multipleOf") && schema.at("multipleOf").is_integer();
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto maximum = schema.at("maximum");
    auto multiplier = schema.at("multipleOf");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", std::move(multiplier));
    make_encoding(schema, "ROOF_MULTIPLE_MIRROR_ENUM_VARINT", options);
  }
};
