class TypeUnionImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeUnionImplicit()
      : SchemaTransformRule{
            "type_union_implicit",
            "Not setting `type` is equivalent to accepting any type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    using namespace sourcemeta::core;
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Validation,
         Vocabularies::Known::JSON_Schema_2019_09_Validation,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1,
         Vocabularies::Known::JSON_Schema_Draft_0}));
    ONLY_CONTINUE_IF(!schema.defines("type"));
    ONLY_CONTINUE_IF(!schema.defines("enum"));
    ONLY_CONTINUE_IF(!vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) ||
                     !schema.defines("const"));

    for (const auto &entry : schema.as_object()) {
      const auto &keyword_type{walker(entry.first, vocabularies).type};

      // References point to other schemas that may have type constraints
      ONLY_CONTINUE_IF(keyword_type != SchemaKeywordType::Reference);

      // Logical in-place applicators apply without affecting the instance
      // location, meaning they impose constraints on the same instance. Adding
      // an implicit type union alongside these would create redundant branches
      // that need complex simplification
      ONLY_CONTINUE_IF(
          keyword_type != SchemaKeywordType::ApplicatorValueOrElementsInPlace &&
          keyword_type != SchemaKeywordType::ApplicatorMembersInPlaceSome &&
          keyword_type != SchemaKeywordType::ApplicatorElementsInPlace &&
          keyword_type != SchemaKeywordType::ApplicatorElementsInPlaceSome &&
          keyword_type !=
              SchemaKeywordType::ApplicatorElementsInPlaceSomeNegate &&
          keyword_type != SchemaKeywordType::ApplicatorValueInPlaceMaybe &&
          keyword_type != SchemaKeywordType::ApplicatorValueInPlaceNegate);
    }

    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto types{sourcemeta::core::JSON::make_array()};

    types.push_back(sourcemeta::core::JSON{"null"});
    types.push_back(sourcemeta::core::JSON{"boolean"});
    types.push_back(sourcemeta::core::JSON{"object"});
    types.push_back(sourcemeta::core::JSON{"array"});
    types.push_back(sourcemeta::core::JSON{"string"});

    // Note we don't add `integer`, as its covered by `number`
    types.push_back(sourcemeta::core::JSON{"number"});

    schema.assign("type", std::move(types));
  }
};
