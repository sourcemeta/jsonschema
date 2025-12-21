class UnknownLocalRef final : public SchemaTransformRule {
public:
  UnknownLocalRef()
      : SchemaTransformRule{
            "unknown_local_ref",
            "Local references that point to unknown locations are invalid and "
            "will result in evaluation failures"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &frame, const SchemaFrame::Location &location,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Core,
         Vocabularies::Known::JSON_Schema_2019_09_Core,
         // In JSON Schema Draft 7 and older, `$ref` overrides siblings.
         // However, we do not need to worry about this case here, as if the
         // `$ref` points to an unknown local location, the entire schema is
         // invalid anyway. We just help at least making the schema valid
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("$ref") &&
                     schema.at("$ref").is_string());

    // Find the keyword location entry
    const auto absolute_ref_pointer{location.pointer.concat({"$ref"})};
    const auto reference_entry{frame.references().find(
        {SchemaReferenceType::Static, absolute_ref_pointer})};
    ONLY_CONTINUE_IF(reference_entry != frame.references().end());

    // If the keyword has no fragment, continue
    const auto &reference_fragment{reference_entry->second.fragment};
    ONLY_CONTINUE_IF(reference_fragment.has_value());

    // Only continue if the reference target does not exist
    const auto target_location{frame.locations().find(
        {SchemaReferenceType::Static, reference_entry->second.destination})};
    ONLY_CONTINUE_IF(target_location == frame.locations().end());

    // If there is a base beyond the fragment, the base must exist.
    // Otherwise it is likely an external reference?
    const auto &reference_base{reference_entry->second.base};
    if (reference_base.has_value()) {
      const auto base_location{frame.locations().find(
          {SchemaReferenceType::Static, reference_base.value()})};
      ONLY_CONTINUE_IF(base_location != frame.locations().end());
    }

    return APPLIES_TO_KEYWORDS("$ref");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$ref");
  }
};
