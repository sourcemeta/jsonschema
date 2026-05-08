class DropExtendsEmptySchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DropExtendsEmptySchemas()
      : SchemaTransformRule{
            "drop_extends_empty_schemas",
            "Empty schemas in `extends` are redundant and can be removed"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3) &&
        schema.is_object());

    const auto *extends{schema.try_at("extends")};
    ONLY_CONTINUE_IF(extends);

    if (sourcemeta::core::is_empty_schema(*extends)) {
      return APPLIES_TO_POINTERS({Pointer{"extends"}});
    }

    if (extends->is_array() && !extends->empty()) {
      std::vector<Pointer> locations;
      for (std::size_t index = 0; index < extends->size(); ++index) {
        if (sourcemeta::core::is_empty_schema(extends->at(index))) {
          locations.push_back(Pointer{"extends", index});
        }
      }
      ONLY_CONTINUE_IF(!locations.empty());
      return APPLIES_TO_POINTERS(std::move(locations));
    }

    return false;
  }

  auto transform(JSON &schema, const Result &result) const -> void override {
    if (result.locations.size() == 1 && result.locations.at(0).size() == 1) {
      schema.erase("extends");
      return;
    }

    auto new_extends{JSON::make_array()};
    for (const auto &entry : schema.at("extends").as_array()) {
      if (!sourcemeta::core::is_empty_schema(entry)) {
        new_extends.push_back(entry);
      }
    }

    if (new_extends.empty()) {
      schema.erase("extends");
    } else {
      schema.assign("extends", std::move(new_extends));
    }
  }
};
