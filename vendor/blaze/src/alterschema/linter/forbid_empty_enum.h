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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && !schema.defines("not"));

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     enum_value->empty());
    ONLY_CONTINUE_IF(!frame.has_references_through(location.pointer));
    return APPLIES_TO_KEYWORDS("enum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("enum").into(JSON::make_object());
    schema.rename("enum", "not");
  }
};
