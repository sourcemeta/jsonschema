class SingleTypeArray final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
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
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_1,
                          Vocabularies::Known::JSON_Schema_Draft_0}) &&
                     schema.is_object() && schema.defines("type") &&
                     schema.at("type").is_array() &&
                     schema.at("type").size() == 1 &&
                     schema.at("type").front().is_string());
    return APPLIES_TO_KEYWORDS("type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto type{schema.at("type").front()};
    schema.at("type").into(std::move(type));
  }
};
