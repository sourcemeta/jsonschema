class DependenciesDefault final : public SchemaTransformRule {
public:
  DependenciesDefault()
      : SchemaTransformRule{
            "dependencies_default",
            "Setting the `dependencies` keyword to an empty object "
            "does not add any further constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(vocabularies,
                        {"http://json-schema.org/draft-07/schema#",
                         "http://json-schema.org/draft-06/schema#",
                         "http://json-schema.org/draft-04/schema#",
                         "http://json-schema.org/draft-03/schema#"}) &&
           schema.is_object() && schema.defines("dependencies") &&
           schema.at("dependencies").is_object() &&
           schema.at("dependencies").empty();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("dependencies");
  }
};
