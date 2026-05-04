class PortableAnchorNames final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  PortableAnchorNames()
      : SchemaTransformRule{
            "portable_anchor_names",
            "Keep anchors within the safe allowed character set across JSON "
            "Schema dialects (`^[A-Za-z][A-Za-z0-9_.-]*$`)"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Core,
         Vocabularies::Known::JSON_Schema_2019_09_Core,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4}));
    ONLY_CONTINUE_IF(schema.is_object());

    std::vector<Pointer> offenders;

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Core,
             Vocabularies::Known::JSON_Schema_2019_09_Core})) {
      this->check_anchor_keyword(schema, ANCHOR, offenders);
    }

    if (vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Core})) {
      this->check_anchor_keyword(schema, DYNAMIC_ANCHOR, offenders);
    }

    if (vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_7,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_4})) {
      const auto &id_keyword{
          vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_4})
              ? ID_DRAFT_4
              : ID_MODERN};
      this->check_id_fragment(schema, id_keyword, offenders);
    }

    ONLY_CONTINUE_IF(!offenders.empty());
    return APPLIES_TO_POINTERS(std::move(offenders));
  }

private:
  static inline const sourcemeta::core::JSON::String ANCHOR{"$anchor"};
  static inline const sourcemeta::core::JSON::String DYNAMIC_ANCHOR{
      "$dynamicAnchor"};
  static inline const sourcemeta::core::JSON::String ID_MODERN{"$id"};
  static inline const sourcemeta::core::JSON::String ID_DRAFT_4{"id"};
  static inline const Regex SAFE_ANCHOR_PATTERN{
      to_regex("^[A-Za-z][A-Za-z0-9_.-]*$").value()};

  static auto
  check_anchor_keyword(const sourcemeta::core::JSON &schema,
                       const sourcemeta::core::JSON::String &keyword,
                       std::vector<Pointer> &offenders) -> void {
    if (!schema.defines(keyword) || !schema.at(keyword).is_string()) {
      return;
    }

    const auto &value{schema.at(keyword).to_string()};
    if (value.empty()) {
      return;
    }

    if (!matches(SAFE_ANCHOR_PATTERN, value)) {
      offenders.push_back(Pointer{keyword});
    }
  }

  static auto check_id_fragment(const sourcemeta::core::JSON &schema,
                                const sourcemeta::core::JSON::String &keyword,
                                std::vector<Pointer> &offenders) -> void {
    if (!schema.defines(keyword) || !schema.at(keyword).is_string()) {
      return;
    }

    const auto &value{schema.at(keyword).to_string()};
    if (value.find('#') == sourcemeta::core::JSON::String::npos) {
      return;
    }

    const sourcemeta::core::URI uri{value};
    const auto fragment{uri.fragment()};
    if (!fragment.has_value() || fragment.value().empty()) {
      return;
    }

    if (!matches(SAFE_ANCHOR_PATTERN,
                 sourcemeta::core::JSON::String{fragment.value()})) {
      offenders.push_back(Pointer{keyword});
    }
  }
};
