class DuplicateTypeEntries final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DuplicateTypeEntries() : SchemaTransformRule{"duplicate_type_entries"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
        schema.is_object());

    // Rewriting a type name into its subschema form can leave two entries that
    // differ only in the form they were written in, which the meta-schema
    // rejects for asking that the entries be unique
    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array() && !type->unique());

    // Compacting the array would shift the index of every entry that follows a
    // removed duplicate, so a reference into `type` could silently end up
    // pointing at a different subschema
    const std::string keyword{"type"};
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(keyword)}));

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    const auto &original{schema.at("type")};

    std::unordered_set<
        std::reference_wrapper<const sourcemeta::core::JSON>,
        sourcemeta::core::HashJSON<
            std::reference_wrapper<const sourcemeta::core::JSON>>,
        sourcemeta::core::EqualJSON<
            std::reference_wrapper<const sourcemeta::core::JSON>>>
        seen;
    auto result{sourcemeta::core::JSON::make_array()};

    for (const auto &element : original.as_array()) {
      if (seen.emplace(std::cref(element)).second) {
        result.push_back(element);
      }
    }

    schema.assign("type", std::move(result));
  }
};
