class SingleTypeArray final : public SchemaTransformRule {
public:
  SingleTypeArray()
      : SchemaTransformRule{"single_type_array",
                            "Setting `type` to an array of a single type is "
                            "the same as directly declaring such type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/schema#",
                "http://json-schema.org/draft-01/schema#",
                "http://json-schema.org/draft-00/schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_array() && schema.at("type").size() == 1 &&
           schema.at("type").front().is_string();
  }

  auto transform(JSON &schema) const -> void override {
    auto type{schema.at("type").front()};
    schema.at("type").into(std::move(type));
  }
};
