class UnnecessaryAllOfWrapperProperties final : public SchemaTransformRule {
public:
  UnnecessaryAllOfWrapperProperties()
      : SchemaTransformRule{
            "unnecessary_allof_wrapper_properties",
            "Avoid unnecessarily wrapping object `properties` in `allOf` as it "
            "may introduce a minor evaluation performance overhead and even "
            "confuse documentation generators"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(
            vocabularies,
            // TODO: Make this work on older dialects. Right we can't do that
            // safely for Draft 7 and earlier if `properties` is a sibling of
            // `$ref`
            {"https://json-schema.org/draft/2020-12/vocab/applicator",
             "https://json-schema.org/draft/2019-09/vocab/applicator"}) &&
        schema.is_object() && schema.defines("allOf") &&
        schema.at("allOf").is_array() && schema.defines("properties") &&
        schema.at("properties").is_object());

    std::size_t cursor{0};
    for (const auto &entry : schema.at("allOf").as_array()) {
      if (entry.is_object() && entry.defines("properties") &&
          entry.at("properties").is_object()) {
        for (const auto &subentry : entry.at("properties").as_object()) {
          if (!schema.at("properties").defines(subentry.first)) {
            return APPLIES_TO_POINTERS(
                {{"allOf", cursor, "properties", subentry.first}});
          }
        }
      }

      cursor += 1;
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    for (auto &entry : schema.at("allOf").as_array()) {
      if (entry.is_object() && entry.defines("properties")) {
        std::vector<JSON::String> blacklist;
        for (const auto &subentry : entry.at("properties").as_object()) {
          if (!schema.at("properties").defines(subentry.first)) {
            blacklist.push_back(subentry.first);
          }
        }

        for (auto &property : blacklist) {
          schema.at("properties")
              .assign(property, entry.at("properties").at(property));
          entry.at("properties").erase(property);
        }

        if (entry.at("properties").empty()) {
          entry.erase("properties");
        }
      }
    }

    schema.at("allOf").erase_if(sourcemeta::core::is_empty_schema);

    if (schema.at("allOf").empty()) {
      schema.erase("allOf");
    }
  }
};
