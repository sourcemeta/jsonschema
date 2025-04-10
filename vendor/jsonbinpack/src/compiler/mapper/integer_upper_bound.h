class IntegerUpperBound final : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerUpperBound()
      : sourcemeta::core::SchemaTransformRule{"integer_upper_bound", ""} {};

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
           !schema.defines("multipleOf");
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto maximum = schema.at("maximum");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("maximum", std::move(maximum));
    options.assign("multiplier", sourcemeta::core::JSON{1});
    make_encoding(schema, "ROOF_MULTIPLE_MIRROR_ENUM_VARINT", options);
  }
};
