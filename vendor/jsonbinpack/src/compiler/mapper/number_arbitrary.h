class NumberArbitrary final : public sourcemeta::core::SchemaTransformRule {
public:
  NumberArbitrary()
      : sourcemeta::core::SchemaTransformRule{"number_arbitrary", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") && schema.at("type").to_string() == "number";
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    make_encoding(schema, "DOUBLE_VARINT_TUPLE",
                  sourcemeta::core::JSON::make_object());
  }
};
