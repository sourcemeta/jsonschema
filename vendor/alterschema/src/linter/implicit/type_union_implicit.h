class TypeUnionImplicit final : public sourcemeta::alterschema::Rule {
public:
  TypeUnionImplicit()
      : Rule{"type_union_implicit",
             "Not setting `type` is equivalent to accepting any type"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    if (!schema.is_object()) {
      return false;
    }

    if (contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/hyper-schema#",
                      "http://json-schema.org/draft-01/hyper-schema#",
                      "http://json-schema.org/draft-00/hyper-schema#"})) {
      if (schema.defines("type")) {
        return false;
      }

      // Don't apply if we don't have the necessary vocabularies
    } else {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/core") &&
        schema.defines_any({"$ref", "$dynamicRef"})) {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/applicator") &&
        schema.defines_any(
            {"anyOf", "oneOf", "allOf", "if", "then", "else", "not"})) {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/validation") &&
        schema.defines_any({"enum", "const"})) {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2019-09/vocab/core") &&
        schema.defines_any({"$ref", "$recursiveRef"})) {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2019-09/vocab/applicator") &&
        schema.defines_any(
            {"anyOf", "oneOf", "allOf", "if", "then", "else", "not"})) {
      return false;
    }

    if (vocabularies.contains(
            "https://json-schema.org/draft/2019-09/vocab/validation") &&
        schema.defines_any({"enum", "const"})) {
      return false;
    }

    if (vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
        schema.defines_any({"$ref", "enum", "const", "anyOf", "oneOf", "allOf",
                            "if", "then", "else", "not"})) {
      return false;
    }

    if (vocabularies.contains("http://json-schema.org/draft-06/schema#") &&
        schema.defines_any(
            {"$ref", "enum", "const", "anyOf", "oneOf", "allOf", "not"})) {
      return false;
    }

    if (vocabularies.contains("http://json-schema.org/draft-04/schema#") &&
        schema.defines_any(
            {"$ref", "enum", "anyOf", "oneOf", "allOf", "not"})) {
      return false;
    }

    if (vocabularies.contains("http://json-schema.org/draft-03/schema#") &&
        schema.defines_any({"$ref", "enum", "disallow", "extends"})) {
      return false;
    }

    if (vocabularies.contains(
            "http://json-schema.org/draft-02/hyper-schema#") &&
        schema.defines_any({"enum", "disallow", "extends"})) {
      return false;
    }

    if (vocabularies.contains(
            "http://json-schema.org/draft-01/hyper-schema#") &&
        schema.defines_any({"enum", "disallow", "extends"})) {
      return false;
    }

    if (vocabularies.contains(
            "http://json-schema.org/draft-00/hyper-schema#") &&
        schema.defines_any({"enum", "disallow", "extends"})) {
      return false;
    }

    return true;
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    auto types{sourcemeta::jsontoolkit::JSON::make_array()};

    // All possible JSON Schema types
    // See
    // https://json-schema.org/draft/2020-12/json-schema-validation.html#rfc.section.6.1.1
    types.push_back(sourcemeta::jsontoolkit::JSON{"null"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"boolean"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"object"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"array"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"string"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"number"});
    types.push_back(sourcemeta::jsontoolkit::JSON{"integer"});

    transformer.assign("type", std::move(types));
  }
};
