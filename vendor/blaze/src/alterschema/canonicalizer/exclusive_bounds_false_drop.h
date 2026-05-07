class ExclusiveBoundsFalseDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExclusiveBoundsFalseDrop()
      : SchemaTransformRule{"exclusive_bounds_false_drop", ""} {};

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

    const auto *exclusive_min{schema.try_at("exclusiveMinimum")};
    this->has_exclusive_min_ = exclusive_min && exclusive_min->is_boolean() &&
                               !exclusive_min->to_boolean();
    const auto *exclusive_max{schema.try_at("exclusiveMaximum")};
    this->has_exclusive_max_ = exclusive_max && exclusive_max->is_boolean() &&
                               !exclusive_max->to_boolean();

    ONLY_CONTINUE_IF(this->has_exclusive_min_ || this->has_exclusive_max_);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (this->has_exclusive_min_) {
      schema.erase("exclusiveMinimum");
    }

    if (this->has_exclusive_max_) {
      schema.erase("exclusiveMaximum");
    }
  }

private:
  mutable bool has_exclusive_min_{false};
  mutable bool has_exclusive_max_{false};
};
