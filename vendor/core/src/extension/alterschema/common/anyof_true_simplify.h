class AnyOfTrueSimplify final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AnyOfTrueSimplify()
      : SchemaTransformRule{
            "anyof_true_simplify",
            "An `anyOf` with a `true` or `{}` branch always succeeds"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array());

    const auto &anyof{schema.at(KEYWORD)};
    for (std::size_t index = 0; index < anyof.size(); ++index) {
      const auto &entry{anyof.at(index)};
      if ((entry.is_boolean() && entry.to_boolean()) ||
          (entry.is_object() && entry.empty())) {
        ONLY_CONTINUE_IF(!frame.has_references_through(
            location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
        return APPLIES_TO_POINTERS({Pointer{KEYWORD, index}});
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("anyOf");
  }
};
