class UnnecessaryExtendsWrapper final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD{"extends"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnnecessaryExtendsWrapper()
      : SchemaTransformRule{"unnecessary_extends_wrapper",
                            "Keywords inside `extends` that do not conflict "
                            "with the parent schema can be elevated"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &frame, const SchemaFrame::Location &location,
            const SchemaWalker &walker, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper}));
    ONLY_CONTINUE_IF(schema.is_object());

    const auto *extends_value{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(extends_value && extends_value->is_array() &&
                     !extends_value->empty());

    std::unordered_set<std::string_view> dependency_blocked;
    for (const auto &entry : schema.as_object()) {
      for (const auto &dependency :
           walker(entry.first, vocabularies).dependencies) {
        dependency_blocked.emplace(dependency);
      }
    }

    const auto *parent_type_value{schema.try_at("type")};
    const JSON::TypeSet parent_types{
        parent_type_value && is_known_type_form(*parent_type_value)
            ? parse_schema_type(*parent_type_value)
            : JSON::TypeSet{}};

    const auto &extends{*extends_value};
    std::vector<Pointer> locations;
    std::unordered_set<std::string_view> elevated;

    for (auto index = extends.size(); index > 0; index--) {
      const auto &entry{extends.at(index - 1)};
      if (!entry.is_object() || entry.empty() ||
          // We separately handle this case via
          // `unnecessary_extends_ref_wrapper`
          entry.defines("$ref")) {
        continue;
      }

      auto entry_pointer{location.pointer};
      entry_pointer.push_back(std::cref(KEYWORD));
      entry_pointer.push_back(index - 1);
      if (frame.has_references_to(entry_pointer)) {
        continue;
      }

      // Skip entries that define their own identity, as elevating keywords
      // from them could break references that target those identifiers
      if (entry.defines("id")) {
        continue;
      }

      for (const auto &keyword_entry : entry.as_object()) {
        const auto &keyword{keyword_entry.first};
        const auto &metadata{walker(keyword, vocabularies)};

        if (elevated.contains(keyword) ||
            (schema.defines(keyword) &&
             schema.at(keyword) != keyword_entry.second)) {
          continue;
        }

        if (dependency_blocked.contains(keyword)) {
          continue;
        }

        if (metadata.instances.any() && parent_types.any() &&
            (metadata.instances & parent_types).none()) {
          continue;
        }

        if (std::ranges::any_of(
                metadata.dependencies, [&](const auto &dependency) {
                  return !entry.defines(std::string{dependency}) &&
                         (schema.defines(std::string{dependency}) ||
                          elevated.contains(dependency));
                })) {
          continue;
        }

        locations.push_back(Pointer{KEYWORD, index - 1, keyword});
        elevated.emplace(keyword);

        for (const auto &dependency : metadata.dependencies) {
          if (!entry.defines(std::string{dependency})) {
            dependency_blocked.emplace(dependency);
          }
        }
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      assert(location.size() == 3);
      const auto extends_index{location.at(1).to_index()};
      const auto &keyword{location.at(2).to_property()};
      schema.try_assign_before(
          keyword, schema.at(KEYWORD).at(extends_index).at(keyword), KEYWORD);
      schema.at(KEYWORD).at(extends_index).erase(keyword);
    }
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    // The rule moves keywords from /extends/<index>/<keyword> to /<keyword>
    const auto extends_prefix{current.concat({KEYWORD})};
    const auto relative{target.resolve_from(extends_prefix)};
    const auto &keyword{relative.at(1).to_property()};
    const Pointer old_prefix{extends_prefix.concat({relative.at(0), keyword})};
    const Pointer new_prefix{current.concat({keyword})};
    return target.rebase(old_prefix, new_prefix);
  }

private:
  static auto is_known_type_form(const sourcemeta::core::JSON &type) -> bool {
    if (type.is_string()) {
      return type.to_string() != "any";
    }
    if (!type.is_array()) {
      return false;
    }
    return std::ranges::all_of(type.as_array(), [](const auto &entry) {
      return entry.is_string() && entry.to_string() != "any";
    });
  }
};
