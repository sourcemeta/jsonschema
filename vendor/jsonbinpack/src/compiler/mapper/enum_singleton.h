class EnumSingleton final : public sourcemeta::core::SchemaTransformRule {
public:
  EnumSingleton()
      : sourcemeta::core::SchemaTransformRule{"enum_singleton", ""} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &dialect,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return dialect == "https://json-schema.org/draft/2020-12/schema" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.defines("enum") && schema.at("enum").is_array() &&
           schema.at("enum").size() == 1;
  }

  auto transform(sourcemeta::core::PointerProxy &transformer) const
      -> void override {
    auto options = sourcemeta::core::JSON::make_object();
    options.assign("value", transformer.value().at("enum").at(0));
    make_encoding(transformer, "CONST_NONE", options);
  }
};
