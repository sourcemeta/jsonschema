class AllOfFalseSimplify final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AllOfFalseSimplify()
      : SchemaTransformRule{"allof_false_simplify",
                            "When `allOf` contains a `false` branch, the "
                            "schema is unsatisfiable"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"allOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     !schema.defines("not") && schema.at(KEYWORD).is_array());

    const auto &allof{schema.at(KEYWORD)};
    for (std::size_t index = 0; index < allof.size(); ++index) {
      const auto &entry{allof.at(index)};
      if (entry.is_boolean() && !entry.to_boolean()) {
        ONLY_CONTINUE_IF(!frame.has_references_through(
            location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
        return APPLIES_TO_POINTERS({Pointer{KEYWORD, index}});
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("allOf").into(JSON{true});
    schema.rename("allOf", "not");
  }
};
