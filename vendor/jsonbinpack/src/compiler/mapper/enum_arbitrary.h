// TODO: Unit test this mapping once we have container encodings
class EnumArbitrary final : public sourcemeta::blaze::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumArbitrary()
      : sourcemeta::blaze::SchemaTransformRule{"enum_arbitrary", ""} {};

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
           schema.is_object() && schema.defines("enum") &&
           schema.at("enum").is_array() && !location.pointer.empty() &&
           schema.at("enum").size() > 1 &&
           !sourcemeta::core::is_byte(schema.at("enum").size() - 1);
  }

  auto transform(
      sourcemeta::core::JSON &schema,
      [[maybe_unused]] const sourcemeta::blaze::SchemaTransformRule::Result
          &result) const -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("choices", schema.at("enum"));
    make_encoding(schema, "LARGE_CHOICE_INDEX", options);
  }
};
