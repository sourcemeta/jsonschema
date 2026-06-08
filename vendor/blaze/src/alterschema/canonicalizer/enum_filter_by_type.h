class EnumFilterByType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumFilterByType() : SchemaTransformRule{"enum_filter_by_type", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_0,
             Vocabularies::Known::JSON_Schema_Draft_1,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string());
    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     !enum_value->empty());

    const auto declared_types{parse_schema_type(*type)};
    ONLY_CONTINUE_IF(declared_types.any());
    const bool integer_matches_integral{
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        declared_types.test(std::to_underlying(JSON::Type::Integer))};

    this->matching_indices_.clear();
    bool has_mismatch{false};
    std::size_t index{0};
    for (const auto &value : enum_value->as_array()) {
      const bool matches{
          declared_types.test(std::to_underlying(value.type())) ||
          (integer_matches_integral && value.is_integral())};
      if (matches) {
        this->matching_indices_.push_back(index);
      } else {
        has_mismatch = true;
      }
      index++;
    }

    ONLY_CONTINUE_IF(!this->matching_indices_.empty() && has_mismatch);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto filtered{JSON::make_array()};
    for (const auto &index : this->matching_indices_) {
      filtered.push_back(schema.at("enum").at(index));
    }

    schema.assign("enum", std::move(filtered));
  }

private:
  mutable std::vector<std::size_t> matching_indices_;
};
