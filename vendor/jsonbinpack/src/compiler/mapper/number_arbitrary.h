class NumberArbitrary final : public sourcemeta::blaze::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NumberArbitrary()
      : sourcemeta::blaze::SchemaTransformRule{"number_arbitrary", ""} {};

  [[nodiscard]] auto condition(
      const sourcemeta::core::JSON &schema,
      [[maybe_unused]] const sourcemeta::core::JSON &root,
      const sourcemeta::core::SchemaVocabularies &vocabularies,
      [[maybe_unused]] const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      [[maybe_unused]] const sourcemeta::core::SchemaWalker &walker,
      [[maybe_unused]] const sourcemeta::core::SchemaResolver &resolver) const
      -> sourcemeta::blaze::SchemaTransformRule::Result override {
    return location.dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(sourcemeta::core::SchemaVocabularies::Known::
                                     JSON_SCHEMA_2020_12_VALIDATION) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").to_string() == "number";
  }

  auto transform(
      sourcemeta::core::JSON &schema,
      [[maybe_unused]] const sourcemeta::blaze::SchemaTransformRule::Result
          &result) const -> void override {
    make_encoding(schema, "DOUBLE_VARINT_TUPLE",
                  sourcemeta::core::JSON::make_object());
  }
};
