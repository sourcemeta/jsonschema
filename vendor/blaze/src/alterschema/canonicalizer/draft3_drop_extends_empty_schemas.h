class Draft3DropExtendsEmptySchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  Draft3DropExtendsEmptySchemas()
      : SchemaTransformRule{"draft3_drop_extends_empty_schemas", ""} {};

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
      return true;
    }

    if (extends->is_array() && !extends->empty()) {
      return std::ranges::any_of(extends->as_array(),
                                 sourcemeta::core::is_empty_schema);
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const auto &extends{schema.at("extends")};
    if (sourcemeta::core::is_empty_schema(extends)) {
      schema.erase("extends");
      return;
    }

    auto new_extends{JSON::make_array()};
    for (const auto &entry : extends.as_array()) {
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
