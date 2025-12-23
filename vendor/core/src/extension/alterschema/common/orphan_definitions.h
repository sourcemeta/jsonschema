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
    const bool has_modern_core{
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core)};
    const bool has_draft_definitions{
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_7) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_6) ||
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_4)};

    ONLY_CONTINUE_IF(has_modern_core || has_draft_definitions);
    ONLY_CONTINUE_IF(schema.is_object());

    std::vector<Pointer> orphans;

    if (has_modern_core) {
      collect_orphans(frame, location, schema, "$defs", orphans);
    }

    if (has_modern_core || has_draft_definitions) {
      collect_orphans(frame, location, schema, "definitions", orphans);
    }

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
  collect_orphans(const sourcemeta::core::SchemaFrame &frame,
                  const sourcemeta::core::SchemaFrame::Location &root,
                  const JSON &schema, const JSON::String &container,
                  std::vector<Pointer> &orphans) -> void {
    if (!schema.defines(container) || !schema.at(container).is_object()) {
      return;
    }

    for (const auto &entry : schema.at(container).as_object()) {
      auto entry_pointer{Pointer{container, entry.first}};
      const auto &entry_location{frame.traverse(root, entry_pointer)};
      if (frame.instance_locations(entry_location).empty()) {
        orphans.push_back(std::move(entry_pointer));
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
