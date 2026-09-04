class NumberArbitrary final : public sourcemeta::blaze::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NumberArbitrary()
      : sourcemeta::blaze::SchemaTransformRule{"number_arbitrary", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            [[maybe_unused]] const sourcemeta::core::JSON &root,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            [[maybe_unused]] const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            [[maybe_unused]] const sourcemeta::blaze::SchemaWalker &walker,
            [[maybe_unused]] const sourcemeta::blaze::SchemaResolver &resolver,
            [[maybe_unused]] const bool is_metaschema) const
      -> sourcemeta::blaze::SchemaTransformRule::Result override {
    return location.dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(sourcemeta::blaze::SchemaVocabularies::Known::
                                     JSON_Schema_2020_12_Validation) &&
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
