class BooleanTrue final : public Rule {
public:
  BooleanTrue()
      : Rule{"boolean_true",
             "The boolean schema `true` is syntax sugar for the empty schema"} {
        };

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return schema.is_boolean() && schema.to_boolean();
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.replace(sourcemeta::jsontoolkit::JSON::make_object());
  }
};
