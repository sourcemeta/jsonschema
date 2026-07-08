class AllOfFalseSimplify final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  AllOfFalseSimplify() : SchemaTransformRule{"allof_false_simplify"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"allOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && !schema.defines("not"));

    const auto *all_of{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(all_of && all_of->is_array());

    for (std::size_t index = 0; index < all_of->size(); ++index) {
      const auto &entry{all_of->at(index)};
      if (entry.is_boolean() && !entry.to_boolean()) {
        ONLY_CONTINUE_IF(!frame.has_references_through(
            location.pointer,
            sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.at("allOf").into(sourcemeta::core::JSON{true});
    schema.rename("allOf", "not");
  }
};
