class NumberArbitrary final : public sourcemeta::blaze::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NumberArbitrary()
      : sourcemeta::blaze::SchemaTransformRule{"number_arbitrary", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> sourcemeta::blaze::SchemaTransformRule::Result override {
    return location.dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(sourcemeta::blaze::Vocabularies::Known::
                                     JSON_Schema_2020_12_Validation) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").to_string() == "number";
  }

  auto transform(sourcemeta::core::JSON &schema,
                 const sourcemeta::blaze::SchemaTransformRule::Result &) const
      -> void override {
    make_encoding(schema, "DOUBLE_VARINT_TUPLE",
                  sourcemeta::core::JSON::make_object());
  }
};
