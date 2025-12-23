class UnknownKeywordsPrefix final : public SchemaTransformRule {
public:
  UnknownKeywordsPrefix()
      : SchemaTransformRule{
            "unknown_keywords_prefix",
            "Future versions of JSON Schema will refuse to evaluate unknown "
            "keywords or custom keywords from optional vocabularies that don't "
            "have an x- prefix"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &walker, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(schema.is_object());
    std::vector<Pointer> locations;
    for (const auto &entry : schema.as_object()) {
      if (entry.first.starts_with("x-")) {
        continue;
      }

      const auto &metadata = walker(entry.first, vocabularies);
      if (metadata.type == SchemaKeywordType::Unknown &&
          // If there is any i.e. optional vocabulary we don't recognise, then
          // this seemingly unknown keyword might belong to one of those, and
          // thus it might not be safe to flag it
          !vocabularies.has_unknown()) {
        locations.push_back(Pointer{entry.first});
      }
    }

    ONLY_CONTINUE_IF(!locations.empty());
    return APPLIES_TO_POINTERS(std::move(locations));
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    for (const auto &location : result.locations) {
      const auto &keyword{location.at(0).to_property()};
      assert(schema.defines(keyword));
      std::string prefixed_name = "x-" + keyword;
      while (schema.defines(prefixed_name)) {
        prefixed_name.insert(0, "x-");
      }

      schema.rename(keyword, std::move(prefixed_name));
    }
  }
};
