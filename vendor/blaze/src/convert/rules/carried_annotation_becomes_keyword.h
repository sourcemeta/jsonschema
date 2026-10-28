// Give back the name of an annotation the OpenAPI 3.0 fork parked, now that
// every rung that would have promoted it is behind the document.
//
// This runs in a phase of its own rather than among the rungs. A rung reads
// the dialect in front of it and this reads what the fork left behind, so one
// scheduled alongside the other would hand a rung the very keyword the parking
// was there to keep from it
class CarriedAnnotationBecomesKeyword final : public SchemaTransformRule {
public:
  CarriedAnnotationBecomesKeyword()
      : SchemaTransformRule{"carried_annotation_becomes_keyword"} {};

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

      // Nothing can be standing here. The fork took this name's own member
      // away as it parked it, and no rung writes one back
      sourcemeta::core::JSON::String restored{annotation};
      this->renames_.emplace_back(sourcemeta::core::Pointer{parked},
                                  sourcemeta::core::Pointer{restored});
      schema.rename(parked, std::move(restored));
    }
  }

  [[nodiscard]] auto relocations() const -> std::vector<Relocation> override {
    return this->renames_;
  }

private:
  mutable std::vector<Relocation> renames_;
};
