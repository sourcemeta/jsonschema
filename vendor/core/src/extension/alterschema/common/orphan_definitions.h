class OrphanDefinitions final : public SchemaTransformRule {
public:
  OrphanDefinitions()
      : SchemaTransformRule{
            "orphan_definitions",
            "Schema definitions in `$defs` or `definitions` that "
            "are never internally referenced can be removed"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());
    const bool has_modern_core{
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core)};
    const bool has_draft_definitions{
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_7) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_6) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_4)};
    const bool has_defs{has_modern_core && schema.defines("$defs")};
    const bool has_definitions{(has_modern_core || has_draft_definitions) &&
                               schema.defines("definitions")};
    ONLY_CONTINUE_IF(has_defs || has_definitions);

    const auto prefix_size{location.pointer.size()};
    bool has_external_to_defs{false};
    bool has_external_to_definitions{false};
    std::unordered_set<std::string_view> outside_referenced_defs;
    std::unordered_set<std::string_view> outside_referenced_definitions;

    for (const auto &[key, reference] : frame.references()) {
      const auto destination_location{frame.traverse(reference.destination)};
      if (destination_location.has_value()) {
        if (has_defs) {
          process_reference(key.second, destination_location->get().pointer,
                            location.pointer, prefix_size, "$defs",
                            has_external_to_defs, outside_referenced_defs);
        }

        if (has_definitions) {
          process_reference(key.second, destination_location->get().pointer,
                            location.pointer, prefix_size, "definitions",
                            has_external_to_definitions,
                            outside_referenced_definitions);
        }
      }
    }

    std::vector<Pointer> orphans;
    collect_orphans(schema, "$defs", has_defs, has_external_to_defs,
                    outside_referenced_defs, orphans);
    collect_orphans(schema, "definitions", has_definitions,
                    has_external_to_definitions, outside_referenced_definitions,
                    orphans);

    ONLY_CONTINUE_IF(!orphans.empty());
    return APPLIES_TO_POINTERS(std::move(orphans));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &pointer : result.locations) {
      assert(pointer.size() == 2);
      assert(pointer.at(0).is_property());
      assert(pointer.at(1).is_property());
      const auto &container{pointer.at(0).to_property()};
      schema.at(container).erase(pointer.at(1).to_property());
    }

    remove_empty_container(schema, "$defs");
    remove_empty_container(schema, "definitions");
  }

private:
  static auto
  process_reference(const Pointer &source_pointer,
                    const Pointer &destination_pointer, const Pointer &prefix,
                    const std::size_t prefix_size, std::string_view container,
                    bool &has_external,
                    std::unordered_set<std::string_view> &referenced) -> void {
    if (!destination_pointer.starts_with(prefix, container) ||
        destination_pointer.size() <= prefix_size + 1) {
      return;
    }

    const auto &entry_token{destination_pointer.at(prefix_size + 1)};
    if (entry_token.is_property()) {
      const auto &entry_name{entry_token.to_property()};
      if (!source_pointer.starts_with(prefix, container)) {
        has_external = true;
        referenced.insert(entry_name);
      } else if (!source_pointer.starts_with(prefix, container, entry_name)) {
        referenced.insert(entry_name);
      }
    }
  }

  static auto
  collect_orphans(const JSON &schema, const JSON::String &container,
                  const bool has_container, const bool has_external_reference,
                  const std::unordered_set<std::string_view> &referenced,
                  std::vector<Pointer> &orphans) -> void {
    if (has_container) {
      const auto &maybe_object{schema.at(container)};
      if (maybe_object.is_object()) {
        // If no external references to container, all definitions are orphans
        // Otherwise, only unreferenced definitions are orphans
        for (const auto &entry : maybe_object.as_object()) {
          if (!has_external_reference || !referenced.contains(entry.first)) {
            orphans.push_back(Pointer{container, entry.first});
          }
        }
      }
    }
  }

  static auto remove_empty_container(JSON &schema, const JSON::String &name)
      -> void {
    if (schema.defines(name) && schema.at(name).empty()) {
      schema.erase(name);
    }
  }
};
