class UpgradeDraft4ToDraft6 final : public SchemaTransformRule {
public:
  UpgradeDraft4ToDraft6()
      : SchemaTransformRule{"upgrade_draft_4_to_draft_6"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4) &&
        schema.is_object());

    ONLY_CONTINUE_IF(has_pending_draft_4_pattern(schema, site.dialect) ||
                     at_dialect_declaration(schema, site));

    // Anchors are renamed document-wide by a rule of its own, and bumping the
    // dialect is what stops `id` from identifying anything, so this waits for
    // that to finish rather than deciding which resource ought to do it
    return !this->anchors_pending_;
  }

  auto plan(const sourcemeta::core::JSON &, const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &, const Site &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> void override {
    this->anchors_pending_ = has_unsanitized_draft_4_anchor(root, frame);
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void override {
    if (schema.defines("id") && schema.at("id").is_string()) {
      schema.rename("id", "$id");
    }

    if (schema.defines("exclusiveMinimum") &&
        schema.at("exclusiveMinimum").is_boolean()) {
      const bool exclusive{schema.at("exclusiveMinimum").to_boolean()};
      schema.erase("exclusiveMinimum");
      if (exclusive && schema.defines("minimum") &&
          schema.at("minimum").is_number()) {
        schema.rename("minimum", "exclusiveMinimum");
      }
    }

    if (schema.defines("exclusiveMaximum") &&
        schema.at("exclusiveMaximum").is_boolean()) {
      const bool exclusive{schema.at("exclusiveMaximum").to_boolean()};
      schema.erase("exclusiveMaximum");
      if (exclusive && schema.defines("maximum") &&
          schema.at("maximum").is_number()) {
        schema.rename("maximum", "exclusiveMaximum");
      }
    }

    bump_dialect(schema, site, DRAFT_6_URL);
  }

private:
  mutable bool anchors_pending_{false};

  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_4_URL{
      "http://json-schema.org/draft-04/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string DRAFT_6_URL{
      "http://json-schema.org/draft-06/schema#"};
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::array<std::string_view, 4> PROMOTED_KEYWORDS{
      {"const", "contains", "propertyNames", "examples"}};

  static auto
  has_pending_draft_4_pattern(const sourcemeta::core::JSON &subschema,
                              const std::string_view dialect) -> bool {
    if (!subschema.is_object()) {
      return false;
    }

    // What framing reads is what decides whether this rung still has work
    // here. A `$schema` that framing does not read declares nothing, and
    // taking it at its word would put the subschema out of reach while it is
    // still waiting to be converted, letting an ancestor move the dialect out
    // from under it
    if (dialect_position(dialect) > dialect_position(DRAFT_4_URL)) {
      return false;
    }

    if (subschema.defines("$schema") && subschema.at("$schema").is_string() &&
        subschema.at("$schema").to_string() == DRAFT_4_URL) {
      return true;
    }

    if (subschema.defines("id") && subschema.at("id").is_string()) {
      const auto fragment{identifier_fragment(subschema.at("id"))};
      if (!fragment.has_value() || fragment.value().empty() ||
          is_draft_6_plain_name(fragment.value())) {
        return true;
      }
    }

    const auto *exclusive_minimum{subschema.try_at("exclusiveMinimum")};
    if (exclusive_minimum != nullptr && exclusive_minimum->is_boolean()) {
      return true;
    }

    const auto *exclusive_maximum{subschema.try_at("exclusiveMaximum")};
    if (exclusive_maximum != nullptr && exclusive_maximum->is_boolean()) {
      return true;
    }

    for (const auto &keyword : PROMOTED_KEYWORDS) {
      if (subschema.defines(keyword)) {
        return true;
      }
    }

    return has_stray_identifier(subschema, dialect);
  }

  // Draft 4 does not know `$id`, so one written there is inert data that
  // Draft 6 would read as the identifier, and it has to be shadowed before
  // `id` takes that name. It is also the one Draft 6 addition this rule
  // produces itself, so unlike every other promoted keyword its presence only
  // means work is pending while the subschema is still on Draft 4 or older.
  //
  // How the subschema is read counts as well as what it declares. A resource
  // around it may have moved on while leaving it declaring nothing, and the
  // `$id` is then the identifier doing its job rather than data awaiting a
  // shadow. Asking only what it declares leaves an ancestor waiting on a
  // subschema that nothing is going to change again
  static auto has_stray_identifier(const sourcemeta::core::JSON &subschema,
                                   const std::string_view dialect) -> bool {
    return subschema.defines("$id") &&
           dialect_position(dialect) <= dialect_position(DRAFT_4_URL) &&
           dialect_position(declared_dialect(subschema)) <=
               dialect_position(DRAFT_4_URL);
  }
};
