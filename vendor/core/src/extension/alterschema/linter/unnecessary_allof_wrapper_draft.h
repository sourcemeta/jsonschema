class UnnecessaryAllOfWrapperDraft final : public SchemaTransformRule {
public:
  UnnecessaryAllOfWrapperDraft()
      : SchemaTransformRule{"unnecessary_allof_wrapper_draft",
                            "Wrapping any keyword other than `$ref` in `allOf` "
                            "is unnecessary and may even introduce a minor "
                            "evaluation performance overhead"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(contains_any(vocabularies,
                                  {"http://json-schema.org/draft-07/schema#",
                                   "http://json-schema.org/draft-06/schema#",
                                   "http://json-schema.org/draft-04/schema#"}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("allOf") &&
                     schema.at("allOf").is_array());

    std::vector<Pointer> locations;
    const auto &all_of{schema.at("allOf")};
    bool multi_ref_only{all_of.size() > 1};
    for (std::size_t index = 0; index < all_of.size(); index++) {
      const auto &entry{all_of.at(index)};
      if (entry.is_object()) {
        // It is dangerous to extract type-specific keywords from a schema that
        // declares a type into another schema that also declares a type if
        // the types are different. As we might lead to those type-keywords
        // getting incorrectly removed if they don't apply to the target type
        if (schema.defines("type") && entry.defines("type") &&
            // TODO: Ideally we also check for intersection of types in type
            // arrays or whether one is contained in the other
            schema.at("type") != entry.at("type")) {
          multi_ref_only = false;
          continue;
        }

        for (const auto &subentry : entry.as_object()) {
          if (walker(subentry.first, vocabularies).type !=
              SchemaKeywordType::Reference) {
            multi_ref_only = false;
          }

          if (subentry.first != "$ref" && !schema.defines(subentry.first)) {
            locations.push_back(Pointer{"allOf", index, subentry.first});
          }
        }
      }
    }

    ONLY_CONTINUE_IF(!locations.empty() && !multi_ref_only);
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    for (auto &entry : schema.at("allOf").as_array()) {
      if (entry.is_object()) {
        std::vector<JSON::String> blacklist;
        for (auto &subentry : entry.as_object()) {
          if (subentry.first != "$ref" && !schema.defines(subentry.first)) {
            blacklist.push_back(subentry.first);
          }
        }

        for (auto &property : blacklist) {
          schema.try_assign_before(property, entry.at(property), "allOf");
          entry.erase(property);
        }
      }
    }

    schema.at("allOf").erase_if(sourcemeta::core::is_empty_schema);

    if (schema.at("allOf").empty()) {
      schema.erase("allOf");
    }
  }
};
