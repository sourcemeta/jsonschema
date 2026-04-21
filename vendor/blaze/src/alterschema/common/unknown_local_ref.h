class UnknownLocalRef final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD{"$ref"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
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
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_string());

    // Find the keyword location entry
    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    const auto reference_entry{
        frame.reference(SchemaReferenceType::Static, keyword_pointer)};
    ONLY_CONTINUE_IF(reference_entry.has_value());

    // If the keyword has no fragment, continue
    const auto &reference_fragment{reference_entry->get().fragment};
    ONLY_CONTINUE_IF(reference_fragment.has_value());

    // Only continue if the reference target does not exist
    ONLY_CONTINUE_IF(
        !frame.traverse(reference_entry->get().destination).has_value());

    // If there is a base beyond the fragment, the base must exist.
    // Otherwise it is likely an external reference?
    const auto &reference_base{reference_entry->get().base};
    if (!reference_base.empty()) {
      ONLY_CONTINUE_IF(frame.traverse(reference_base).has_value());
    }

    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
