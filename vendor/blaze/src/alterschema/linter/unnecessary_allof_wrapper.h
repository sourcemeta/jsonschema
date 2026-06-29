class UnnecessaryAllOfWrapper final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"allOf"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnnecessaryAllOfWrapper()
      : SchemaTransformRule{"unnecessary_allof_wrapper",
                            "Keywords inside `allOf` that do not conflict with "
                            "the parent schema can be elevated"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &frame, const SchemaFrame::Location &location,
            const SchemaWalker &walker, const SchemaResolver &,
            const bool) const -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
         Vocabularies::Known::JSON_Schema_2019_09_Applicator,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4}));
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *all_of_value{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(all_of_value && all_of_value->is_array() &&
                     !all_of_value->empty());

    std::unordered_map<std::string_view, std::size_t> keyword_frequency;
    for (const auto &entry : all_of_value->as_array()) {
      if (!entry.is_object()) {
        continue;
      }
      for (const auto &property : entry.as_object()) {
        const auto &metadata{walker(property.first, vocabularies)};
        if (metadata.type == SchemaKeywordType::Annotation ||
            metadata.type == SchemaKeywordType::Comment) {
          continue;
        }
        keyword_frequency[property.first]++;
      }
    }

    std::unordered_set<std::string_view> dependency_blocked;
    for (const auto &entry : schema.as_object()) {
      if ((entry.first == "unevaluatedProperties" ||
           entry.first == "unevaluatedItems") &&
          vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator})) {
        continue;
      }

      for (const auto &dependency :
           walker(entry.first, vocabularies).dependencies) {
        dependency_blocked.emplace(dependency);
      }
    }

    const auto *parent_type_value{schema.try_at("type")};
    const JSON::TypeSet parent_types{
        parent_type_value &&
                vocabularies.contains_any(
                    {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                     Vocabularies::Known::JSON_Schema_2019_09_Validation,
                     Vocabularies::Known::JSON_Schema_Draft_7,
                     Vocabularies::Known::JSON_Schema_Draft_6,
                     Vocabularies::Known::JSON_Schema_Draft_4})
            ? parse_schema_type(*parent_type_value)
            : JSON::TypeSet{}};

    const auto &all_of{*all_of_value};
    std::vector<Pointer> locations;
    std::unordered_set<std::string_view> elevated;

    for (auto index = all_of.size(); index > 0; index--) {
      const auto &entry{all_of.at(index - 1)};
      if (!entry.is_object() || entry.empty() ||
          // We separately handle this case, as it has many other subtleties
          entry.defines("$ref")) {
        continue;
      }

      // Skip entries that have direct references pointing to them
      auto entry_pointer{location.pointer};
      entry_pointer.push_back(std::cref(KEYWORD));
      entry_pointer.push_back(index - 1);
      if (frame.has_references_to(entry_pointer)) {
        continue;
      }

      // Skip entries that define their own identity, as elevating keywords
      // from them could break references that target those anchors
      if (!this->is_anonymous(entry, vocabularies)) {
        continue;
      }

      if (vocabularies.contains_any(
              {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
               Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
          (entry.defines("unevaluatedProperties") ||
           entry.defines("unevaluatedItems"))) {
        continue;
      }

      const auto try_elevate_keyword = [&](const auto &keyword_entry) -> bool {
        const auto &keyword{keyword_entry.first};
        const auto &metadata{walker(keyword, vocabularies)};

        if (elevated.contains(keyword) ||
            (schema.defines(keyword) &&
             schema.at(keyword) != keyword_entry.second)) {
          return false;
        }

        if (dependency_blocked.contains(keyword)) {
          return false;
        }

        if (keyword_frequency[keyword] > 1) {
          return false;
        }

        if (metadata.instances.any() && parent_types.any() &&
            (metadata.instances & parent_types).none()) {
          return false;
        }

        if (std::ranges::any_of(metadata.dependencies,
                                [&](const auto &dependency) -> auto {
                                  return !entry.defines(dependency) &&
                                         (schema.defines(dependency) ||
                                          elevated.contains(dependency));
                                })) {
          return false;
        }

        locations.push_back(Pointer{KEYWORD, index - 1, keyword});
        elevated.emplace(keyword);

        if (!(vocabularies.contains_any(
                  {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
                   Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
              (keyword == "unevaluatedProperties" ||
               keyword == "unevaluatedItems"))) {
          for (const auto &dependency : metadata.dependencies) {
            if (!entry.defines(dependency)) {
              dependency_blocked.emplace(dependency);
            }
          }
        }

        return true;
      };

      bool entry_has_non_annotation = false;
      bool non_annotation_elevated = false;
      for (const auto &keyword_entry : entry.as_object()) {
        const auto &metadata{walker(keyword_entry.first, vocabularies)};
        if (metadata.type == SchemaKeywordType::Annotation ||
            metadata.type == SchemaKeywordType::Comment) {
          continue;
        }
        entry_has_non_annotation = true;
        if (try_elevate_keyword(keyword_entry)) {
          non_annotation_elevated = true;
        }
      }

      if (!entry_has_non_annotation || non_annotation_elevated) {
        for (const auto &keyword_entry : entry.as_object()) {
          const auto &metadata{walker(keyword_entry.first, vocabularies)};
          if (metadata.type != SchemaKeywordType::Annotation &&
              metadata.type != SchemaKeywordType::Comment) {
            continue;
          }
          try_elevate_keyword(keyword_entry);
        }
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      assert(location.size() == 3);
      const auto allof_index{location.at(1).to_index()};
      const auto &keyword{location.at(2).to_property()};
      schema.try_assign_before(
          keyword, schema.at(KEYWORD).at(allof_index).at(keyword), KEYWORD);
      schema.at(KEYWORD).at(allof_index).erase(keyword);
    }
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    // The rule moves keywords from /allOf/<index>/<keyword> to /<keyword>
    const auto allof_prefix{current.concat(KEYWORD)};
    const auto relative{target.resolve_from(allof_prefix)};
    const auto &keyword{relative.at(1).to_property()};
    const Pointer old_prefix{
        allof_prefix.concat(Pointer{relative.at(0), keyword})};
    const Pointer new_prefix{current.concat(keyword)};
    return target.rebase(old_prefix, new_prefix);
  }

private:
  // TODO: Ideally we this information from the frame out of the box
  [[nodiscard]] auto is_anonymous(const JSON &entry,
                                  const Vocabularies &vocabularies) const
      -> bool {
    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Core,
             Vocabularies::Known::JSON_Schema_2019_09_Core})) {
      if (entry.defines("$id") || entry.defines("$anchor")) {
        return false;
      }

      if (vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2020_12_Core) &&
          entry.defines("$dynamicAnchor")) {
        return false;
      }

      if (vocabularies.contains(
              Vocabularies::Known::JSON_Schema_2019_09_Core) &&
          entry.defines("$recursiveAnchor") &&
          entry.at("$recursiveAnchor").is_boolean() &&
          entry.at("$recursiveAnchor").to_boolean()) {
        return false;
      }

      return true;
    }

    if (vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_7,
                                   Vocabularies::Known::JSON_Schema_Draft_6})) {
      return !entry.defines("$id");
    }

    if (vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_4)) {
      return !entry.defines("id");
    }

    return false;
  }
};
