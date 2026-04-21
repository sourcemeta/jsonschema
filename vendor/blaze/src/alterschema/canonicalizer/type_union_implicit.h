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
      -> SchemaTransformRule::Result override {
    using namespace sourcemeta::core;
    ONLY_CONTINUE_IF(schema.is_object() && !schema.empty());
    ONLY_CONTINUE_IF(!vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_0,
                          Vocabularies::Known::JSON_Schema_Draft_1,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_3}) ||
                     !schema.defines("disallow"));
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

      ONLY_CONTINUE_IF(keyword_type != SchemaKeywordType::Reference);
      ONLY_CONTINUE_IF(
          // Applicators like `contentSchema` applies to decoded content, not
          // the current instance
          keyword_type == SchemaKeywordType::ApplicatorValueInPlaceOther ||
          !IS_IN_PLACE_APPLICATOR(keyword_type));
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
