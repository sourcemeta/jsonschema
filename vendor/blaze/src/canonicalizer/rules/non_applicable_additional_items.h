class NonApplicableAdditionalItems final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"additionalItems"};

public:
  using reframe_after_transform = std::true_type;
  NonApplicableAdditionalItems()
      : SchemaTransformRule{"non_applicable_additional_items"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object() && schema.defines(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));

    const auto *items{schema.try_at("items")};
    return ((items != nullptr) &&
            (items->is_object() || items->is_boolean())) ||
           (items == nullptr);
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase(KEYWORD);
  }
};
