class UnnecessaryAllOfRefWrapperModern final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnnecessaryAllOfRefWrapperModern()
      : SchemaTransformRule{"unnecessary_allof_ref_wrapper_modern",
                            "Wrapping `$ref` in `allOf` was only necessary in "
                            "JSON Schema Draft 7 and older"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
         Vocabularies::Known::JSON_Schema_2019_09_Applicator}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("allOf") &&
                     schema.at("allOf").is_array());

    const auto &all_of{schema.at("allOf")};

    // Don't do anything if there is more than one branch and ALL branches
    // define `$ref` (a common multiple composition pattern)
    ONLY_CONTINUE_IF(
        !(all_of.size() > 1 &&
          std::ranges::all_of(all_of.as_array(), [](const auto &entry) {
            return entry.is_object() && entry.defines("$ref");
          })));

    std::vector<Pointer> locations;
    for (std::size_t index = 0; index < all_of.size(); index++) {
      const auto &entry{all_of.at(index)};
      if (entry.is_object() && entry.defines("$ref") &&
          // We cannot safely elevate a reference on a subschema with its own
          // base URI
          // TODO: In theory we can if the URI is absolute
          !entry.defines("$id") && !schema.defines("$ref")) {
        locations.push_back(Pointer{"allOf", index, "$ref"});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      assert(location.size() == 3);
      const auto allof_index{location.at(1).to_index()};
      const auto &keyword{location.at(2).to_property()};

      if (!schema.defines(keyword)) {
        schema.try_assign_before(
            keyword, schema.at("allOf").at(allof_index).at(keyword), "allOf");
        schema.at("allOf").at(allof_index).erase(keyword);
      }
    }

    schema.at("allOf").erase_if(sourcemeta::core::is_empty_schema);

    if (schema.at("allOf").empty()) {
      schema.erase("allOf");
    }
  }
};
