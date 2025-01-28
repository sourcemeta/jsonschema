class IntegerUpperBoundMultiplier final
    : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerUpperBoundMultiplier()
      : sourcemeta::core::SchemaTransformRule{"integer_upper_bound_multiplier",
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
           !schema.defines("minimum") && schema.defines("maximum") &&
           schema.defines("multipleOf") && schema.at("multipleOf").is_integer();
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto maximum = transformer.value().at("maximum");
    auto multiplier = transformer.value().at("multipleOf");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", std::move(multiplier));
    make_encoding(transformer, "ROOF_MULTIPLE_MIRROR_ENUM_VARINT", options);
  }
};
