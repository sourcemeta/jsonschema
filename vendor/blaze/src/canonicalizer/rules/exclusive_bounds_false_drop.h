class ExclusiveBoundsFalseDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExclusiveBoundsFalseDrop()
      : SchemaTransformRule{"exclusive_bounds_false_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));

    std::vector<sourcemeta::core::Pointer> locations;
    const auto *exclusive_min{schema.try_at("exclusiveMinimum")};
    if (exclusive_min && exclusive_min->is_boolean() &&
        !exclusive_min->to_boolean()) {
      locations.push_back(sourcemeta::core::Pointer{"exclusiveMinimum"});
    }
    const auto *exclusive_max{schema.try_at("exclusiveMaximum")};
    if (exclusive_max && exclusive_max->is_boolean() &&
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
