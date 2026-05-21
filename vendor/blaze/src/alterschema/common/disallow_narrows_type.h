class DisallowNarrowsType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DisallowNarrowsType()
      : SchemaTransformRule{
            "disallow_narrows_type",
            "When `disallow` excludes types that are also in the parent "
            "`type`, those types can be removed from `type` and the "
            "corresponding `disallow` entries dropped"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"disallow"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *disallow{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(disallow && disallow->is_array() && !disallow->empty());

    const auto *parent_type{schema.try_at("type")};
    ONLY_CONTINUE_IF(parent_type && parent_type->is_array() &&
                     parent_type->size() > 1);

    std::unordered_set<JSON::String> parent_type_names;
    for (const auto &entry : parent_type->as_array()) {
      ONLY_CONTINUE_IF(entry.is_string() && entry.to_string() != "any");
      parent_type_names.insert(entry.to_string());
    }

    std::vector<Pointer> locations;
    std::unordered_set<JSON::String> narrowed_types;
    for (std::size_t index = 0; index < disallow->size(); ++index) {
      const auto entry_types{extract_type_names(disallow->at(index))};
      if (entry_types.empty()) {
        continue;
      }

      const bool all_in_parent{std::ranges::all_of(
          entry_types, [&parent_type_names](const auto &type_name) {
            return parent_type_names.contains(type_name);
          })};
      if (!all_in_parent) {
        continue;
      }

      locations.push_back(Pointer{KEYWORD, index});
      narrowed_types.insert(entry_types.cbegin(), entry_types.cend());
    }

    ONLY_CONTINUE_IF(!locations.empty());
    ONLY_CONTINUE_IF(narrowed_types.size() < parent_type_names.size());

    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(keyword_pointer));

    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    std::unordered_set<JSON::String> narrowed_types;
    std::vector<std::size_t> dead_indices;
    dead_indices.reserve(result.locations.size());
    const auto &disallow{schema.at("disallow")};
    for (const auto &location : result.locations) {
      assert(location.size() == 2);
      const auto index{location.at(1).to_index()};
      dead_indices.push_back(index);
      const auto entry_types{extract_type_names(disallow.at(index))};
      narrowed_types.insert(entry_types.cbegin(), entry_types.cend());
    }

    auto new_type{JSON::make_array()};
    for (const auto &entry : schema.at("type").as_array()) {
      if (entry.is_string() && !narrowed_types.contains(entry.to_string())) {
        new_type.push_back(entry);
      }
    }
    schema.assign("type", std::move(new_type));

    auto new_disallow{JSON::make_array()};
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

private:
  static auto extract_type_names(const sourcemeta::core::JSON &entry)
      -> std::unordered_set<JSON::String> {
    std::unordered_set<JSON::String> result;
    if (entry.is_string()) {
      if (entry.to_string() != "any") {
        result.insert(entry.to_string());
      }
      return result;
    }
    if (!entry.is_object() || entry.size() != 1) {
      return result;
    }
    const auto *entry_type{entry.try_at("type")};
    if (!entry_type) {
      return result;
    }
    if (entry_type->is_string()) {
      if (entry_type->to_string() != "any") {
        result.insert(entry_type->to_string());
      }
      return result;
    }
    if (!entry_type->is_array()) {
      return result;
    }
    for (const auto &type_entry : entry_type->as_array()) {
      if (!type_entry.is_string() || type_entry.to_string() == "any") {
        return std::unordered_set<JSON::String>{};
      }
      result.insert(type_entry.to_string());
    }
    return result;
  }
};
