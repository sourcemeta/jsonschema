class DuplicateExamples final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  DuplicateExamples()
      : SchemaTransformRule{
            "duplicate_examples",
            "Setting duplicate values in `examples` is redundant"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
                          Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object());

    const auto *examples{schema.try_at("examples")};
    ONLY_CONTINUE_IF(examples && examples->is_array() && !examples->unique());
    return APPLIES_TO_KEYWORDS("examples");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    std::unordered_set<JSON, HashJSON<JSON>> seen;
    auto result = JSON::make_array();
    for (const auto &element : schema.at("examples").as_array()) {
      if (!seen.contains(element)) {
        seen.insert(element);
        result.push_back(element);
      }
    }

    schema.at("examples").into(std::move(result));
  }
};
