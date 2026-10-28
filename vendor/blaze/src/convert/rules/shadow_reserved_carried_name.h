// The OpenAPI 3.0 fork parks an annotation under a name of the conversion's
// own while the ladder climbs past the rung that would promote it, and the
// last phase reads it back from there. A member the author wrote under one of
// those names would be read back as a keyword they never asked for, so it is
// moved aside here, before any rung is planned, which is the last moment at
// which the author's member and the conversion's own can still be told apart.
//
// Only the names the fork actually parks under are claimed. The extension
// prefix is the author's namespace and the conversion takes no more of it than
// it writes in
class ShadowReservedCarriedName final : public SchemaTransformRule {
public:
  ShadowReservedCarriedName()
      : SchemaTransformRule{"shadow_reserved_carried_name"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const Site &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> bool override {
    ONLY_CONTINUE_IF(schema.is_object());

    for (const auto &annotation : CARRIED_ANNOTATIONS) {
      if (schema.defines(carried_name(annotation))) {
        return true;
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    this->renames_.clear();
    for (const auto &annotation : CARRIED_ANNOTATIONS) {
      const auto parked{carried_name(annotation)};
      if (!schema.defines(parked)) {
        continue;
      }

      sourcemeta::core::JSON::String shadowed{"x-"};
      shadowed.append(parked);
      while (schema.defines(shadowed)) {
        shadowed.insert(0, "x-");
      }

      this->renames_.emplace_back(sourcemeta::core::Pointer{parked},
                                  sourcemeta::core::Pointer{shadowed});
      schema.rename(parked, std::move(shadowed));
    }
  }

  [[nodiscard]] auto relocations() const -> std::vector<Relocation> override {
    return this->renames_;
  }

private:
  mutable std::vector<Relocation> renames_;
};
