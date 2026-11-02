class FalseToNegation final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  FalseToNegation() : SchemaTransformRule{"false_to_negation"} {};

  // Draft 6 can spell the schema nothing satisfies in two ways: the boolean
  // `false`, and the negation of the schema everything satisfies. A canonical
  // form cannot keep both, or two documents that say the same thing would
  // canonicalise differently. The negation is the one the graph form can hold,
  // since a node body carries keywords and edges rather than a bare boolean
  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const SchemaVocabularies &vocabularies,
                               const SchemaFrame &,
                               const SchemaFrame::Location &,
                               const SchemaWalker &,
                               const SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6}) &&
                     schema.is_boolean() && !schema.to_boolean());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("not", sourcemeta::core::JSON{true});
    schema.into(std::move(result));
  }
};
