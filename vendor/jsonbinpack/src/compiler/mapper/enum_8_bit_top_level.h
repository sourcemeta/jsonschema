class Enum8BitTopLevel final : public sourcemeta::core::SchemaTransformRule {
public:
  Enum8BitTopLevel()
      : sourcemeta::core::SchemaTransformRule{"enum_8_bit_top_level", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &pointer) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("enum") && schema.at("enum").is_array() &&
           pointer.empty() && schema.at("enum").size() > 1 &&
           is_byte(schema.at("enum").size() - 1);
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("choices", transformer.value().at("enum"));
    make_encoding(transformer, "TOP_LEVEL_BYTE_CHOICE_INDEX", options);
  }
};
