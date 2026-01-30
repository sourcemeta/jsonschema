class OrphanDefinitions final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
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
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const
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

    std::vector<Pointer> orphans;
    collect_orphans(frame, walker, resolver, location.pointer, schema, "$defs",
                    has_defs, orphans);
    collect_orphans(frame, walker, resolver, location.pointer, schema,
                    "definitions", has_definitions, orphans);

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

    if (schema.defines("$defs") && schema.at("$defs").empty()) {
      schema.erase("$defs");
    }

    if (schema.defines("definitions") && schema.at("definitions").empty()) {
      schema.erase("definitions");
    }
  }

private:
  static auto has_reachable_reference_through(
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver,
      const WeakPointer &pointer) -> bool {
    for (const auto &reference : frame.references()) {
      const auto destination{frame.traverse(reference.second.destination)};
      if (!destination.has_value()) {
        continue;
      }

      if (!destination->get().pointer.starts_with(pointer)) {
        continue;
      }

      const auto &source_pointer{reference.first.second};
      if (source_pointer.empty()) {
        return true;
      }

      const auto source_location{frame.traverse(
          source_pointer.initial(),
          sourcemeta::core::SchemaFrame::LocationType::Subschema)};
      if (source_location.has_value() &&
          frame.is_reachable(source_location->get(), walker, resolver)) {
        return true;
      }
    }

    return false;
  }

  static auto collect_orphans(const sourcemeta::core::SchemaFrame &frame,
                              const sourcemeta::core::SchemaWalker &walker,
                              const sourcemeta::core::SchemaResolver &resolver,
                              const WeakPointer &prefix, const JSON &schema,
                              const JSON::String &container,
                              const bool has_container,
                              std::vector<Pointer> &orphans) -> void {
    if (!has_container || !schema.at(container).is_object()) {
      return;
    }

    for (const auto &entry : schema.at(container).as_object()) {
      const WeakPointer entry_pointer{std::cref(container),
                                      std::cref(entry.first)};
      const auto absolute_entry_pointer{prefix.concat(entry_pointer)};
      const auto entry_location{frame.traverse(
          absolute_entry_pointer,
          sourcemeta::core::SchemaFrame::LocationType::Subschema)};
      if (entry_location.has_value() &&
          !frame.is_reachable(entry_location->get(), walker, resolver) &&
          !has_reachable_reference_through(frame, walker, resolver,
                                           absolute_entry_pointer)) {
        orphans.push_back(Pointer{container, entry.first});
      }
    }
  }
};
