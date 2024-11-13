class NumberArbitrary final : public sourcemeta::alterschema::Rule {
public:
  NumberArbitrary() : sourcemeta::alterschema::Rule{"number_arbitrary", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") && schema.at("type").to_string() == "number";
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    make_encoding(transformer, "DOUBLE_VARINT_TUPLE",
                  sourcemeta::jsontoolkit::JSON::make_object());
  }
};
