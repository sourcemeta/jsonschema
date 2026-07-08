class DropExtendsEmptySchemas final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DropExtendsEmptySchemas()
      : SchemaTransformRule{"drop_extends_empty_schemas"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    static const sourcemeta::core::JSON::String KEYWORD{"extends"};
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
      this->locations_ = {sourcemeta::core::Pointer{KEYWORD}};
      return true;
    }

    if (extends->is_array() && !extends->empty()) {
      std::vector<sourcemeta::core::Pointer> locations;
      for (std::size_t index = 0; index < extends->size(); ++index) {
        if (sourcemeta::blaze::is_empty_schema(extends->at(index))) {
          locations.push_back(sourcemeta::core::Pointer{KEYWORD, index});
        }
      }
      ONLY_CONTINUE_IF(!locations.empty());
      this->locations_ = std::move(locations);
      return true;
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (this->locations_.size() == 1 && this->locations_.at(0).size() == 1) {
      schema.erase("extends");
      return;
    }

    auto new_extends{sourcemeta::core::JSON::make_array()};
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

private:
  mutable std::vector<sourcemeta::core::Pointer> locations_;
};
