class DuplicateDisallowEntries final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DuplicateDisallowEntries()
      : SchemaTransformRule{
            "duplicate_disallow_entries",
            "Setting duplicate subschemas in `disallow` is redundant, as "
            "negating the same subschema more than once is guaranteed to not "
            "affect the validation result"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
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
        location.pointer, WeakPointer::Token{std::cref(keyword)}));

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const auto &original{schema.at("disallow")};

    std::unordered_set<std::reference_wrapper<const JSON>,
                       HashJSON<std::reference_wrapper<const JSON>>,
                       EqualJSON<std::reference_wrapper<const JSON>>>
        seen;
    auto result{JSON::make_array()};

    for (const auto &element : original.as_array()) {
      if (seen.emplace(std::cref(element)).second) {
        result.push_back(element);
      }
    }

    schema.assign("disallow", std::move(result));
  }
};
