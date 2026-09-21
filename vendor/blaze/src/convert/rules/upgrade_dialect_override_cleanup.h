class UpgradeDialectOverrideCleanup final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  UpgradeDialectOverrideCleanup()
      : SchemaTransformRule{"upgrade_dialect_override_cleanup"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &resolver, const bool) const
      -> bool override {
    ONLY_CONTINUE_IF(location.pointer.empty() && schema.is_object());
    this->redundant_.clear();

    const auto *override_value{schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
    if (override_value != nullptr && override_value->is_string()) {
      return true;
    }

    // A document that names a dialect of its own never bumps a `$schema` the
    // ladder recognises, so nothing there will ever clear the markers that
    // subschemas below it left behind. Every other document clears them when
    // it bumps, and taking them early would strip state the ladder still needs
    ONLY_CONTINUE_IF(dialect_position(declared_dialect(schema)) == 0);

    [[maybe_unused]] const auto visited{frame.any_subschema_under(
        location.pointer,
        [this, &root, &frame, &resolver](
            const sourcemeta::core::SchemaFrame::Location &entry) -> bool {
          const auto entry_pointer{sourcemeta::core::to_pointer(entry.pointer)};
          const auto &entry_schema{sourcemeta::core::get(root, entry_pointer)};
          if (!entry_schema.is_object()) {
            return false;
          }

          const auto *marker{entry_schema.try_at(DIALECT_OVERRIDE_KEYWORD)};
          if (marker == nullptr || !marker->is_string()) {
            return false;
          }

          // Framing agreeing with what the marker claims is what makes it
          // redundant. Until then it is the only record of the move
          const auto vocabulary{marker_vocabulary(marker->to_string())};
          if (vocabulary.has_value() && frame.vocabularies(entry, resolver)
                                            .contains(vocabulary.value())) {
            this->redundant_.push_back(entry_pointer);
          }

          return false;
        })};

    return !this->redundant_.empty();
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (!schema.defines(DIALECT_OVERRIDE_KEYWORD)) {
      for (const auto &pointer : this->redundant_) {
        sourcemeta::core::get(schema, pointer).erase(DIALECT_OVERRIDE_KEYWORD);
      }

      return;
    }

    const sourcemeta::core::JSON::String dialect{
        schema.at(DIALECT_OVERRIDE_KEYWORD).to_string()};
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

    drop_dialect_overrides(schema, true, dialect);
  }

private:
  // The vocabulary that stands for each dialect the ladder walks through, so
  // that what framing reports can be told apart from what a marker claims
  static auto marker_vocabulary(const std::string_view dialect)
      -> std::optional<SchemaVocabularies::Known> {
    using Known = SchemaVocabularies::Known;
    static constexpr std::array<std::pair<std::string_view, Known>, 6> ENTRIES{
        {{"http://json-schema.org/draft-03/schema#",
          Known::JSON_SCHEMA_DRAFT_3},
         {"http://json-schema.org/draft-04/schema#",
          Known::JSON_SCHEMA_DRAFT_4},
         {"http://json-schema.org/draft-06/schema#",
          Known::JSON_SCHEMA_DRAFT_6},
         {"http://json-schema.org/draft-07/schema#",
          Known::JSON_SCHEMA_DRAFT_7},
         {"https://json-schema.org/draft/2019-09/schema",
          Known::JSON_SCHEMA_2019_09_CORE},
         {"https://json-schema.org/draft/2020-12/schema",
          Known::JSON_SCHEMA_2020_12_CORE}}};

    for (const auto &[uri, vocabulary] : ENTRIES) {
      if (uri == dialect) {
        return vocabulary;
      }
    }

    return std::nullopt;
  }

  mutable std::vector<sourcemeta::core::Pointer> redundant_;
};
