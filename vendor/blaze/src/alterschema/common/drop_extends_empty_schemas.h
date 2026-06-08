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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    static const JSON::String KEYWORD{"extends"};
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object());

    const auto *extends{schema.try_at(KEYWORD)};
    ONLY_CONTINUE_IF(extends);

    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(keyword_pointer));

    if (sourcemeta::blaze::is_empty_schema(*extends)) {
      return APPLIES_TO_POINTERS({Pointer{KEYWORD}});
    }

    if (extends->is_array() && !extends->empty()) {
      std::vector<Pointer> locations;
      for (std::size_t index = 0; index < extends->size(); ++index) {
        if (sourcemeta::blaze::is_empty_schema(extends->at(index))) {
          locations.push_back(Pointer{KEYWORD, index});
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
      if (!sourcemeta::blaze::is_empty_schema(entry)) {
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
