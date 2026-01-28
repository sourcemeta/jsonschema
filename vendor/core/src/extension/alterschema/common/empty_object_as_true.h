class EmptyObjectAsTrue final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  EmptyObjectAsTrue()
      : SchemaTransformRule{
            "empty_object_as_true",
            "The empty schema `{}` accepts all values and is equivalent to the "
            "boolean schema `true`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Core,
                          Vocabularies::Known::JSON_Schema_2019_09_Core,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{true});
  }
};
