class AnyOfRemoveFalseSchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  AnyOfRemoveFalseSchemas()
      : SchemaTransformRule{
            "anyof_remove_false_schemas",
            "The boolean schema `false` is guaranteed to never match in "
            "`anyOf`, as it is sufficient for any other branch to match"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"anyOf"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_array() &&
                     schema.at(KEYWORD).contains(JSON{false}));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));

    std::vector<Pointer> false_locations;
    bool has_non_false{false};
    const auto &anyof{schema.at(KEYWORD)};
    for (std::size_t index = 0; index < anyof.size(); ++index) {
      const auto &entry{anyof.at(index)};
      if (entry.is_boolean() && !entry.to_boolean()) {
        false_locations.push_back(Pointer{KEYWORD, index});
      } else {
        has_non_false = true;
      }
    }

    ONLY_CONTINUE_IF(has_non_false);
    return APPLIES_TO_POINTERS(std::move(false_locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    static const JSON::String KEYWORD{"anyOf"};

    std::unordered_set<std::size_t> indices_to_remove;
    for (const auto &location : result.locations) {
      indices_to_remove.insert(location.at(1).to_index());
    }

    auto new_anyof{JSON::make_array()};
    const auto &anyof{schema.at(KEYWORD)};
    for (std::size_t index = 0; index < anyof.size(); ++index) {
      if (!indices_to_remove.contains(index)) {
        new_anyof.push_back(anyof.at(index));
      }
    }

    schema.assign(KEYWORD, std::move(new_anyof));
  }
};
