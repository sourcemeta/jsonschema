class BooleanTrue final : public SchemaTransformRule {
public:
  BooleanTrue()
      : SchemaTransformRule{
            "boolean_true",
            "The boolean schema `true` is syntax sugar for the empty schema"} {
        };

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::Vocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return schema.is_boolean() && schema.to_boolean();
  }

  auto transform(JSON &schema) const -> void override {
    schema.into(sourcemeta::core::JSON::make_object());
  }
};
