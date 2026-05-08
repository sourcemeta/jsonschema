class ExclusiveBoundsFalseDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExclusiveBoundsFalseDrop()
      : SchemaTransformRule{
            "exclusive_bounds_false_drop",
            "Setting `exclusiveMinimum` or `exclusiveMaximum` to `false` "
            "adds no constraint"} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_3,
                                   Vocabularies::Known::JSON_Schema_Draft_4}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));

    std::vector<Pointer> locations;
    const auto *exclusive_min{schema.try_at("exclusiveMinimum")};
    if (exclusive_min && exclusive_min->is_boolean() &&
        !exclusive_min->to_boolean()) {
      locations.push_back(Pointer{"exclusiveMinimum"});
    }
    const auto *exclusive_max{schema.try_at("exclusiveMaximum")};
    if (exclusive_max && exclusive_max->is_boolean() &&
        !exclusive_max->to_boolean()) {
      locations.push_back(Pointer{"exclusiveMaximum"});
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      schema.erase(location.at(0).to_property());
    }
  }
};
