class IncoherentMinMaxContains final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  IncoherentMinMaxContains()
      : SchemaTransformRule{
            "incoherent_min_max_contains",
            "`minContains` greater than `maxContains` makes the schema "
            "unsatisfiable"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("contains") &&
        schema.defines("minContains") &&
        schema.at("minContains").is_integer() &&
        schema.defines("maxContains") &&
        schema.at("maxContains").is_integer() &&
        schema.at("minContains").to_integer() >
            schema.at("maxContains").to_integer());
    return APPLIES_TO_KEYWORDS("minContains", "maxContains");
  }
};
