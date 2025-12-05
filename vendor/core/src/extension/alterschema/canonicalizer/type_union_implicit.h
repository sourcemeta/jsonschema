class TypeUnionImplicit final : public SchemaTransformRule {
public:
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
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
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
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core) ||
        !schema.defines_any({"$ref", "$dynamicRef"}));
    ONLY_CONTINUE_IF(!vocabularies.contains(
                         Vocabularies::Known::JSON_Schema_2020_12_Applicator) ||
                     !schema.defines_any({"anyOf", "oneOf", "allOf", "if",
                                          "then", "else", "not"}));
    ONLY_CONTINUE_IF(!vocabularies.contains(
                         Vocabularies::Known::JSON_Schema_2020_12_Validation) ||
                     !schema.defines_any({"enum", "const"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_2019_09_Core) ||
        !schema.defines_any({"$ref", "$recursiveRef"}));
    ONLY_CONTINUE_IF(!vocabularies.contains(
                         Vocabularies::Known::JSON_Schema_2019_09_Applicator) ||
                     !schema.defines_any({"anyOf", "oneOf", "allOf", "if",
                                          "then", "else", "not"}));
    ONLY_CONTINUE_IF(!vocabularies.contains(
                         Vocabularies::Known::JSON_Schema_2019_09_Validation) ||
                     !schema.defines_any({"enum", "const"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_7) ||
        !schema.defines_any({"$ref", "enum", "const", "anyOf", "oneOf", "allOf",
                             "if", "then", "else", "not"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_6) ||
        !schema.defines_any(
            {"$ref", "enum", "const", "anyOf", "oneOf", "allOf", "not"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_4) ||
        !schema.defines_any(
            {"$ref", "enum", "anyOf", "oneOf", "allOf", "not"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) ||
        !schema.defines_any({"$ref", "enum", "disallow", "extends"}))
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_2) ||
        !schema.defines_any({"enum", "disallow", "extends"}));
    ONLY_CONTINUE_IF(
        !vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_1) ||
        !schema.defines_any({"enum", "disallow", "extends"}));
    ONLY_CONTINUE_IF(!vocabularies.contains(
                         Vocabularies::Known::JSON_Schema_Draft_0_Hyper) ||
                     !schema.defines_any({"enum", "disallow", "extends"}));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto types{sourcemeta::core::JSON::make_array()};

    // All possible JSON Schema types
    // See
    // https://json-schema.org/draft/2020-12/json-schema-validation.html#rfc.section.6.1.1
    types.push_back(sourcemeta::core::JSON{"null"});
    types.push_back(sourcemeta::core::JSON{"boolean"});
    types.push_back(sourcemeta::core::JSON{"object"});
    types.push_back(sourcemeta::core::JSON{"array"});
    types.push_back(sourcemeta::core::JSON{"string"});
    types.push_back(sourcemeta::core::JSON{"number"});
    types.push_back(sourcemeta::core::JSON{"integer"});

    schema.assign("type", std::move(types));
  }
};
