// TODO: Unit test this mapping once we have container encodings
class EnumArbitrary final : public sourcemeta::core::SchemaTransformRule {
public:
  EnumArbitrary()
      : sourcemeta::core::SchemaTransformRule{"enum_arbitrary", ""} {};

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
           schema.defines("enum") && schema.at("enum").is_array() &&
           !location.pointer.empty() && schema.at("enum").size() > 1 &&
           !is_byte(schema.at("enum").size() - 1);
  }

  auto transform(sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaTransformRule::Result &) const
      -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("choices", schema.at("enum"));
    make_encoding(schema, "LARGE_CHOICE_INDEX", options);
  }
};
