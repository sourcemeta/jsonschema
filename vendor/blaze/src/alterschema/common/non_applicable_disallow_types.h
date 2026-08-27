class NonApplicableDisallowTypes final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NonApplicableDisallowTypes()
      : SchemaTransformRule{
            "non_applicable_disallow_types",
            "`disallow` entries whose type cannot overlap with the parent "
            "`type` can never match and can be dropped"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"disallow"};
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
        schema.is_object());

    const auto *disallow{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && !disallow->empty());

    const auto *parent_type_value{schema.try_at("type")};
    ONLY_CONTINUE_IF(parent_type_value &&
                     IS_KNOWN_TYPE_FORM(*parent_type_value, vocabularies));

    const auto parent_types{parse_schema_type(*parent_type_value)};
    ONLY_CONTINUE_IF(parent_types.any());

    std::vector<Pointer> locations;
    for (std::size_t index = 0; index < disallow->size(); ++index) {
      const auto &entry{disallow->at(index)};
      JSON::TypeSet entry_types;
      if (entry.is_string() && entry.to_string() != "any") {
        entry_types = parse_schema_type(entry);
      } else if (entry.is_object()) {
        const auto *entry_type{entry.try_at("type")};
        if (entry_type && IS_KNOWN_TYPE_FORM(*entry_type, vocabularies)) {
          entry_types = parse_schema_type(*entry_type);
        }
      }

      if (entry_types.any() && (parent_types & entry_types).none()) {
        locations.push_back(Pointer{KEYWORD, index});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());

    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(keyword_pointer));

    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    std::vector<std::size_t> dead_indices;
    dead_indices.reserve(result.locations.size());
    for (const auto &location : result.locations) {
      assert(location.size() == 2);
      dead_indices.push_back(location.at(1).to_index());
    }

    auto new_disallow{JSON::make_array()};
    const auto &disallow{schema.at("disallow")};
    for (std::size_t index = 0; index < disallow.size(); ++index) {
      if (std::ranges::find(dead_indices, index) == dead_indices.end()) {
        new_disallow.push_back(disallow.at(index));
      }
    }

    if (new_disallow.empty()) {
      schema.erase("disallow");
    } else {
      schema.assign("disallow", std::move(new_disallow));
    }
  }
};
