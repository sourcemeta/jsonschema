class IntegerLowerBound final : public sourcemeta::core::SchemaTransformRule {
public:
  IntegerLowerBound()
      : sourcemeta::core::SchemaTransformRule{"integer_lower_bound", ""} {};

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
           schema.defines("minimum") && !schema.defines("maximum") &&
           !schema.defines("multipleOf");
  }

  auto transform(sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaTransformRule::Result &) const
      -> void override {
    auto minimum = schema.at("minimum");
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("multiplier", sourcemeta::core::JSON{1});
    make_encoding(schema, "FLOOR_MULTIPLE_ENUM_VARINT", options);
  }
};
