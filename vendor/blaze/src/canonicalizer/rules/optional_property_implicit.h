class OptionalPropertyImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  OptionalPropertyImplicit()
      : SchemaTransformRule{"optional_property_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "object");
    const auto *properties{schema.try_at("properties")};
    ONLY_CONTINUE_IF(properties && properties->is_object());

    for (const auto &entry : properties->as_object()) {
      if (entry.second.is_object() && !entry.second.empty() &&
          !entry.second.defines("optional")) {
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    std::vector<sourcemeta::core::JSON::String> keys;
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
