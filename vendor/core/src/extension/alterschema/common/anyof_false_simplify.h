class AnyOfFalseSimplify final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AnyOfFalseSimplify()
      : SchemaTransformRule{"anyof_false_simplify",
                            "An `anyOf` of a single `false` branch is "
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
    static const JSON::String KEYWORD{"anyOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     !schema.defines("not") && schema.at(KEYWORD).is_array() &&
                     schema.at(KEYWORD).size() == 1);

    const auto &entry{schema.at(KEYWORD).front()};
    ONLY_CONTINUE_IF(entry.is_boolean() && !entry.to_boolean());
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_POINTERS({Pointer{KEYWORD, 0}});
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("anyOf").into(JSON{true});
    schema.rename("anyOf", "not");
  }
};
