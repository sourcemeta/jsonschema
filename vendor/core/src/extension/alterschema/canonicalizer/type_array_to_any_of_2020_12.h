class TypeArrayToAnyOf_2020_12 final : public SchemaTransformRule {
public:
  TypeArrayToAnyOf_2020_12()
      : SchemaTransformRule{
            "type_array_to_any_of_2020_12",
            "Setting `type` to more than one choice is syntax sugar to "
            "`anyOf` over the corresponding types"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,

            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {

    // Note that a big limitation of this rule is that it cannot apply to
    // schemas that have identifiers. For example, consider a schema that has
    // a type union declaration alongside of an `anyOf` where one branch defines
    // `$id` or `$anchor`. We will end up duplicating identifiers (leading to
    // invalid schemas) and there is no silver bullet to avoid these cases.
    const auto has_identifiers{
        std::ranges::any_of(frame.locations(), [](const auto &entry) {
          return entry.second.type ==
                     sourcemeta::core::SchemaFrame::LocationType::Resource ||
                 entry.second.type ==
                     sourcemeta::core::SchemaFrame::LocationType::Anchor;
        })};

    ONLY_CONTINUE_IF(
        contains_any(
            vocabularies,
            {"https://json-schema.org/draft/2020-12/vocab/validation",
             "https://json-schema.org/draft/2020-12/vocab/applicator"}) &&
        !has_identifiers && schema.is_object() && schema.defines("type") &&
        schema.at("type").is_array() &&
        // Non type-specific applicators can leads to invalid schemas
        !schema.defines("$defs") && !schema.defines("$ref") &&
        !schema.defines("if") && !schema.defines("then") &&
        !schema.defines("else") && !schema.defines("allOf") &&
        !schema.defines("oneOf") && !schema.defines("anyOf"));
    return APPLIES_TO_KEYWORDS("type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const std::set<std::string> keep{"$schema", "$id", "$anchor",
                                     "$dynamicAnchor", "$vocabulary"};
    auto disjunctors{sourcemeta::core::JSON::make_array()};
    for (const auto &type : schema.at("type").as_array()) {
      auto copy = schema;
      copy.erase_keys(keep.cbegin(), keep.cend());
      copy.assign("type", type);
      disjunctors.push_back(std::move(copy));
    }

    auto result{sourcemeta::core::JSON::make_object()};
    for (const auto &keyword : keep) {
      if (schema.defines(keyword)) {
        result.assign(keyword, schema.at(keyword));
      }
    }

    result.assign("anyOf", std::move(disjunctors));
    schema.into(std::move(result));
  }
};
