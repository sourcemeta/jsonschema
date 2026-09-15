class ExtendsToArray final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExtendsToArray() : SchemaTransformRule{"extends_to_array"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3}) &&
                     schema.is_object());

    const auto *extends{schema.try_at("extends")};
    ONLY_CONTINUE_IF(extends && !extends->is_array());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto array{sourcemeta::core::JSON::make_array()};
    array.push_back(schema.at("extends"));
    schema.assign("extends", std::move(array));
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    return target.rebase(
        current.concat("extends"),
        current.concat(sourcemeta::core::Pointer{"extends", 0}));
  }
};
