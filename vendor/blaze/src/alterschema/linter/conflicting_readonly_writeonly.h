class ConflictingReadOnlyWriteOnly final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  ConflictingReadOnlyWriteOnly()
      : SchemaTransformRule{"conflicting_readonly_writeonly",
                            "The `readOnly` and `writeOnly` keywords are "
                            "mutually exclusive"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
         Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
         Vocabularies::Known::JSON_Schema_Draft_7}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *read_only{schema.try_at("readOnly")};
    const auto *write_only{schema.try_at("writeOnly")};
    ONLY_CONTINUE_IF(read_only && write_only);
    ONLY_CONTINUE_IF(read_only->is_boolean() && write_only->is_boolean());
    ONLY_CONTINUE_IF(read_only->to_boolean() && write_only->to_boolean());
    return APPLIES_TO_KEYWORDS("readOnly", "writeOnly");
  }
};
