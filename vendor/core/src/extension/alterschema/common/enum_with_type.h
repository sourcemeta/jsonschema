class EnumWithType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumWithType()
      : SchemaTransformRule{
            "enum_with_type",
            "Setting `type` alongside `enum` is considered an anti-pattern, as "
            "the enumeration choices already imply their respective types"} {};

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
         Vocabularies::Known::JSON_Schema_Draft_1}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("type") &&
                     schema.defines("enum") && schema.at("enum").is_array());

    const auto current_types{parse_schema_type(schema.at("type"))};
    ONLY_CONTINUE_IF(std::ranges::all_of(
        schema.at("enum").as_array(), [&current_types](const auto &item) {
          return current_types.test(static_cast<std::size_t>(item.type()));
        }));

    return APPLIES_TO_KEYWORDS("enum", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("type");
  }
};
