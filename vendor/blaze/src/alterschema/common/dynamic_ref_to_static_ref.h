class DynamicRefToStaticRef final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD_DYNAMIC_REF{"$dynamicRef"};
  static inline const std::string KEYWORD_RECURSIVE_REF{"$recursiveRef"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DynamicRefToStaticRef()
      : SchemaTransformRule{
            "dynamic_ref_to_static_ref",
            "A dynamic reference whose destination is unambiguous can be "
            "expressed as a static reference"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object() && !schema.defines("$ref"));

    if (vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core) &&
        schema.defines("$dynamicRef")) {
      auto reference_pointer{location.pointer};
      reference_pointer.push_back(std::cref(KEYWORD_DYNAMIC_REF));

      auto reference_entry{frame.reference(
          sourcemeta::core::SchemaReferenceType::Static, reference_pointer)};
      if (!reference_entry.has_value()) {
        reference_entry = frame.reference(
            sourcemeta::core::SchemaReferenceType::Dynamic, reference_pointer);
      }
      if (!reference_entry.has_value()) {
        return false;
      }

      const auto destination{
          frame.traverse(reference_entry->get().destination)};
      if (!destination.has_value()) {
        return false;
      }

      if (destination->get().type ==
          sourcemeta::core::SchemaFrame::LocationType::Anchor) {
        const auto &subschema{sourcemeta::core::get(
            root, sourcemeta::core::to_pointer(destination->get().pointer))};
        if (subschema.is_object()) {
          const auto *dynamic_anchor{subschema.try_at("$dynamicAnchor")};
          if (dynamic_anchor != nullptr && dynamic_anchor->is_string()) {
            const auto &destination_uri{reference_entry->get().destination};
            const auto fragment_position{destination_uri.find('#')};
            const std::string_view fragment{
                fragment_position == std::string::npos
                    ? std::string_view{destination_uri}
                    : std::string_view{
                          destination_uri.data() + fragment_position + 1,
                          destination_uri.size() - fragment_position - 1}};
            if (fragment == dynamic_anchor->to_string()) {
              return false;
            }
          }
        }
      }

      this->keyword_ = &KEYWORD_DYNAMIC_REF;
      return APPLIES_TO_KEYWORDS("$dynamicRef");
    }

    if (vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) &&
        schema.defines("$recursiveRef")) {
      auto reference_pointer{location.pointer};
      reference_pointer.push_back(std::cref(KEYWORD_RECURSIVE_REF));

      auto reference_entry{frame.reference(
          sourcemeta::core::SchemaReferenceType::Static, reference_pointer)};
      if (!reference_entry.has_value()) {
        reference_entry = frame.reference(
            sourcemeta::core::SchemaReferenceType::Dynamic, reference_pointer);
      }
      if (!reference_entry.has_value()) {
        return false;
      }

      const auto destination{
          frame.traverse(reference_entry->get().destination)};
      if (!destination.has_value()) {
        return false;
      }

      const auto &subschema{sourcemeta::core::get(
          root, sourcemeta::core::to_pointer(destination->get().pointer))};
      if (subschema.is_object()) {
        const auto *recursive_anchor{subschema.try_at("$recursiveAnchor")};
        if (recursive_anchor != nullptr && recursive_anchor->is_boolean() &&
            recursive_anchor->to_boolean()) {
          return false;
        }
      }

      this->keyword_ = &KEYWORD_RECURSIVE_REF;
      return APPLIES_TO_KEYWORDS("$recursiveRef");
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema, const Result &) const
      -> void override {
    assert(this->keyword_ != nullptr);
    schema.rename(*this->keyword_, "$ref");
  }

private:
  mutable const std::string *keyword_{nullptr};
};
