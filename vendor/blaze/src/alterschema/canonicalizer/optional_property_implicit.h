class OptionalPropertyImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  OptionalPropertyImplicit()
      : SchemaTransformRule{"optional_property_implicit", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "object" &&
        schema.defines("properties") && schema.at("properties").is_object());

    for (const auto &entry : schema.at("properties").as_object()) {
      if (entry.second.is_object() && !entry.second.empty() &&
          !entry.second.defines("optional")) {
        return true;
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    std::vector<JSON::String> keys;
    for (const auto &entry : schema.at("properties").as_object()) {
      if (entry.second.is_object() && !entry.second.empty() &&
          !entry.second.defines("optional")) {
        keys.emplace_back(entry.first);
      }
    }
    for (const auto &key : keys) {
      schema.at("properties")
          .at(key)
          .assign("optional", sourcemeta::core::JSON{false});
    }
  }
};
