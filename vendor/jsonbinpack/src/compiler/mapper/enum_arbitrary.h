// TODO: Unit test this mapping once we have container encodings
class EnumArbitrary final : public sourcemeta::core::SchemaTransformRule {
public:
  EnumArbitrary()
      : sourcemeta::core::SchemaTransformRule{"enum_arbitrary", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &pointer) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("enum") && schema.at("enum").is_array() &&
           !pointer.empty() && schema.at("enum").size() > 1 &&
           !is_byte(schema.at("enum").size() - 1);
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("choices", transformer.value().at("enum"));
    make_encoding(transformer, "LARGE_CHOICE_INDEX", options);
  }
};
