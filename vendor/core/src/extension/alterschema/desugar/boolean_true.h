class BooleanTrue final : public SchemaTransformRule {
public:
  BooleanTrue()
      : SchemaTransformRule{
            "boolean_true",
            "The boolean schema `true` is syntax sugar for the empty schema"} {
        };

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return schema.is_boolean() && schema.to_boolean();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.replace(sourcemeta::core::JSON::make_object());
  }
};
