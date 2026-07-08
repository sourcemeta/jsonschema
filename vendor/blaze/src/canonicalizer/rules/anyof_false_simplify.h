class AnyOfFalseSimplify final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  AnyOfFalseSimplify() : SchemaTransformRule{"anyof_false_simplify"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"anyOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && !schema.defines("not"));

    const auto *any_of{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(any_of && any_of->is_array() && any_of->size() == 1);

    const auto &entry{any_of->front()};
    ONLY_CONTINUE_IF(entry.is_boolean() && !entry.to_boolean());
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.at("anyOf").into(sourcemeta::core::JSON{true});
    schema.rename("anyOf", "not");
  }
};
