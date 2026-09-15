class ExclusiveBoundsFalseDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExclusiveBoundsFalseDrop()
      : SchemaTransformRule{"exclusive_bounds_false_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));

    std::vector<sourcemeta::core::Pointer> locations;
    const auto *exclusive_min{schema.try_at("exclusiveMinimum")};
    if ((exclusive_min != nullptr) && exclusive_min->is_boolean() &&
        !exclusive_min->to_boolean()) {
      locations.push_back(sourcemeta::core::Pointer{"exclusiveMinimum"});
    }
    const auto *exclusive_max{schema.try_at("exclusiveMaximum")};
    if ((exclusive_max != nullptr) && exclusive_max->is_boolean() &&
        !exclusive_max->to_boolean()) {
      locations.push_back(sourcemeta::core::Pointer{"exclusiveMaximum"});
    }

    ONLY_CONTINUE_IF(!locations.empty());
    this->locations_ = std::move(locations);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    for (const auto &location : this->locations_) {
      schema.erase(location.at(0).to_property());
    }
  }

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
