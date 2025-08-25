class UnnecessaryAllOfWrapperModern final : public SchemaTransformRule {
public:
  UnnecessaryAllOfWrapperModern()
      : SchemaTransformRule{
            "unnecessary_allof_wrapper_modern",
            "Wrapping any keyword in `allOf` is unnecessary and may even "
            "introduce a minor evaluation performance overhead"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(contains_any(
        vocabularies,
        {"https://json-schema.org/draft/2020-12/vocab/applicator",
         "https://json-schema.org/draft/2019-09/vocab/applicator"}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines("allOf") &&
                     schema.at("allOf").is_array());

    const auto has_validation{contains_any(
        vocabularies,
        {"https://json-schema.org/draft/2020-12/vocab/validation",
         "https://json-schema.org/draft/2019-09/vocab/validation"})};

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
        if (has_validation && schema.defines("type") && entry.defines("type") &&
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

          // TODO: Have another rule that removes a keyword if its exactly
          // equal to an instance of the same keyword outside the wrapper
          if (!schema.defines(subentry.first)) {
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
          if (!schema.defines(subentry.first)) {
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
