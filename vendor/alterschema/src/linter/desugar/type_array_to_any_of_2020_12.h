class TypeArrayToAnyOf_2020_12 final : public Rule {
public:
  TypeArrayToAnyOf_2020_12()
      : Rule{"type_array_to_any_of_2020_12",
             "Setting `type` to more than one choice is syntax sugar to "
             "`anyOf` over the corresponding types"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2020-12/vocab/applicator"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_array() &&
           // Non type-specific applicators can leads to invalid schemas
           !schema.defines("$defs") && !schema.defines("$ref") &&
           !schema.defines("if") && !schema.defines("then") &&
           !schema.defines("else") && !schema.defines("allOf") &&
           !schema.defines("oneOf") && !schema.defines("anyOf");
  }

  auto transform(sourcemeta::alterschema::Transformer &transformer) const
      -> void override {
    const std::set<std::string> keep{"$schema", "$id", "$anchor",
                                     "$dynamicAnchor", "$vocabulary"};
    auto disjunctors{sourcemeta::jsontoolkit::JSON::make_array()};
    for (const auto &type : transformer.schema().at("type").as_array()) {
      auto copy = transformer.schema();
      copy.erase_keys(keep.cbegin(), keep.cend());
      copy.assign("type", type);
      disjunctors.push_back(std::move(copy));
    }

    auto result{sourcemeta::jsontoolkit::JSON::make_object()};
    for (const auto &keyword : keep) {
      if (transformer.schema().defines(keyword)) {
        result.assign(keyword, transformer.schema().at(keyword));
      }
    }

    result.assign("anyOf", std::move(disjunctors));
    transformer.replace(std::move(result));
  }
};
