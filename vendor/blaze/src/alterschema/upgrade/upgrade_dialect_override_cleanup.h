class UpgradeDialectOverrideCleanup final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UpgradeDialectOverrideCleanup()
      : SchemaTransformRule{"upgrade_dialect_override_cleanup", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(location.pointer.empty() && schema.is_object());

    const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    ONLY_CONTINUE_IF(override_value != nullptr && override_value->is_string());

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    if (!schema.defines("$schema")) {
      sourcemeta::core::JSON dialect_value{
          schema.at(DIALECT_OVERRIDE_KEYWORD).to_string()};

      bool placed{false};
      for (const auto &entry : schema.as_object()) {
        if (entry.first != DIALECT_OVERRIDE_KEYWORD) {
          schema.try_assign_before("$schema", dialect_value, entry.first);
          placed = true;
          break;
        }
      }

      if (!placed) {
        schema.assign("$schema", std::move(dialect_value));
      }
    }

    drop_dialect_overrides(schema, true);
  }
};
