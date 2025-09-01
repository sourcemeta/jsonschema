class UnnecessaryAllOfWrapperDraft final : public SchemaTransformRule {
public:
  UnnecessaryAllOfWrapperDraft()
      : SchemaTransformRule{
            "unnecessary_allof_wrapper_draft",
            "Wrapping keywords other than `$ref` in `allOf` "
            "is often unnecessary and may even introduce a minor "
            "evaluation performance overhead"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
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
    bool same_keywords{all_of.size() > 1};
    for (std::size_t index = 0; index < all_of.size(); index++) {
      const auto &entry{all_of.at(index)};
      if (!entry.is_object()) {
        same_keywords = false;
        continue;
      }

      // It is dangerous to extract type-specific keywords from a schema that
      // declares a type into another schema that also declares a type if
      // the types are different. As we might lead to those type-keywords
      // getting incorrectly removed if they don't apply to the target type
      if (schema.defines("type") && entry.defines("type") &&
          // TODO: Ideally we also check for intersection of types in type
          // arrays or whether one is contained in the other
          schema.at("type") != entry.at("type")) {
        same_keywords = false;
        continue;
      }

      for (const auto &subentry : entry.as_object()) {
        // If we have any new keyword in a branch after the first,
        // then we probably need to collapse
        if (index > 0 && same_keywords) {
          const auto &previous{all_of.at(index - 1)};
          if (previous.is_object()) {
            for (const auto &other : previous.as_object()) {
              if (!entry.defines(other.first)) {
                same_keywords = false;
                break;
              }
            }
          }
        }

        if (subentry.first != "$ref" && !schema.defines(subentry.first)) {
          locations.push_back(Pointer{"allOf", index, subentry.first});
        }
      }
    }

    ONLY_CONTINUE_IF(!locations.empty() && !same_keywords);
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
