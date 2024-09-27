class IntegerLowerBoundMultiplier final : public sourcemeta::alterschema::Rule {
public:
  IntegerLowerBoundMultiplier()
      : sourcemeta::alterschema::Rule{"integer_lower_bound_multiplier", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("type") &&
           schema.at("type").to_string() == "integer" &&
           schema.defines("minimum") && !schema.defines("maximum") &&
           schema.defines("multipleOf") && schema.at("multipleOf").is_integer();
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    auto minimum = transformer.schema().at("minimum");
    auto multiplier = transformer.schema().at("multipleOf");
    auto options = sourcemeta::jsontoolkit::JSON::make_object();
    options.assign("minimum", std::move(minimum));
    options.assign("multiplier", std::move(multiplier));
    make_encoding(transformer, "FLOOR_MULTIPLE_ENUM_VARINT", options);
  }
};
