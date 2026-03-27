class ForbidEmptyEnum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ForbidEmptyEnum()
      : SchemaTransformRule{"forbid_empty_enum",
                            "An empty `enum` validates nothing and is "
                            "unsatisfiable"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && !schema.defines("not") &&
                     schema.defines("enum") && schema.at("enum").is_array() &&
                     schema.at("enum").empty());
    ONLY_CONTINUE_IF(!frame.has_references_through(location.pointer));
    return APPLIES_TO_KEYWORDS("enum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("enum").into(JSON::make_object());
    schema.rename("enum", "not");
  }
};
