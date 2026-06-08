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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("contains"));

    const auto *min_contains{schema.try_at("minContains")};
    ONLY_CONTINUE_IF(min_contains && min_contains->is_integer());
    const auto *max_contains{schema.try_at("maxContains")};
    ONLY_CONTINUE_IF(max_contains && max_contains->is_integer() &&
                     min_contains->to_integer() > max_contains->to_integer());
    return APPLIES_TO_KEYWORDS("minContains", "maxContains");
  }
};
