class DuplicateDisallowEntries final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DuplicateDisallowEntries()
      : SchemaTransformRule{"duplicate_disallow_entries"} {};

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
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER}) &&
        schema.is_object());

    const auto *disallow{schema.try_at("disallow")};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && !disallow->unique());

    // Compacting the array would shift the index of every entry that follows a
    // removed duplicate, so a reference into `disallow` could silently end up
    // pointing at a different subschema. Leave such cases untouched and let
    // `DisallowArrayToExtends` split them instead, which preserves every index
    // as its own `extends` branch
    const std::string keyword{"disallow"};
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(keyword)}));

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    const auto &original{schema.at("disallow")};

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

    schema.assign("disallow", std::move(result));
  }
};
