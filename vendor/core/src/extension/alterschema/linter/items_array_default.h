class ItemsArrayDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ItemsArrayDefault()
      : SchemaTransformRule{"items_array_default",
                            "Setting the `items` keyword to the empty array "
                            "does not add any further constraint"} {};

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
                         {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_2_Hyper,
                          Vocabularies::Known::JSON_Schema_Draft_1,
                          Vocabularies::Known::JSON_Schema_Draft_1_Hyper}) &&
                     schema.is_object() && schema.defines("items") &&
                     schema.at("items").is_array() &&
                     schema.at("items").empty());
    return APPLIES_TO_KEYWORDS("items");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("items");
  }
};
