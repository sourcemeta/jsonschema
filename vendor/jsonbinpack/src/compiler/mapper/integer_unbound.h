class IntegerUnbound final : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerUnbound()
      : sourcemeta::core::SchemaTransformRule{"integer_unbound", ""} {};

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
           !schema.defines("multipleOf");
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("multiplier", sourcemeta::core::JSON{1});
    make_encoding(schema, "ARBITRARY_MULTIPLE_ZIGZAG_VARINT", options);
  }
};
