// TODO: Unit test this mapping once we have container encodings
class EnumArbitrary final : public sourcemeta::alterschema::Rule {
public:
  EnumArbitrary() : sourcemeta::alterschema::Rule{"enum_arbitrary", ""} {};

  [[nodiscard]] auto condition(
      const sourcemeta::jsontoolkit::JSON &schema, const std::string &dialect,
      const std::set<std::string> &vocabularies,
      const sourcemeta::jsontoolkit::Pointer &pointer) const -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("enum") && schema.at("enum").is_array() &&
           !pointer.empty() && schema.at("enum").size() > 1 &&
           !is_byte(schema.at("enum").size() - 1);
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    auto options = sourcemeta::jsontoolkit::JSON::make_object();
    options.assign("choices", transformer.schema().at("enum"));
    make_encoding(transformer, "LARGE_CHOICE_INDEX", options);
  }
};
