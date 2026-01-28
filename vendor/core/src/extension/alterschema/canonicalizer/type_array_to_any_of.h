class TypeArrayToAnyOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TypeArrayToAnyOf()
      : SchemaTransformRule{
            "type_array_to_any_of",
            "Setting `type` to more than one choice is syntax sugar to "
            "`anyOf` over the corresponding types"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {

    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("type") &&
                     schema.at("type").is_array());

    this->keyword_instances_.clear();

    for (const auto &entry : schema.as_object()) {
      if (entry.first == "type") {
        continue;
      }

      const auto &metadata{walker(entry.first, vocabularies)};
      if (metadata.instances.any()) {
        this->keyword_instances_[entry.first] = metadata.instances;
      }
    }

    return APPLIES_TO_KEYWORDS("type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto disjunctors{sourcemeta::core::JSON::make_array()};
    for (const auto &type : schema.at("type").as_array()) {
      auto branch{sourcemeta::core::JSON::make_object()};
      branch.assign("type", type);
      const auto current_type_set{parse_schema_type(type)};
      for (const auto &[keyword, instances] : this->keyword_instances_) {
        if ((instances & current_type_set).any()) {
          branch.assign(keyword, schema.at(keyword));
        }
      }

      disjunctors.push_back(std::move(branch));
    }

    for (const auto &[keyword, instances] : this->keyword_instances_) {
      schema.erase(keyword);
    }

    if (schema.defines("anyOf")) {
      auto first_branch{sourcemeta::core::JSON::make_object()};
      first_branch.assign("anyOf", schema.at("anyOf"));
      auto second_branch{sourcemeta::core::JSON::make_object()};
      second_branch.assign("anyOf", std::move(disjunctors));
      schema.erase("anyOf");

      if (schema.defines("allOf")) {
        schema.at("allOf").push_back(std::move(first_branch));
        schema.at("allOf").push_back(std::move(second_branch));
        schema.erase("type");
      } else {
        auto allof_wrapper{sourcemeta::core::JSON::make_array()};
        allof_wrapper.push_back(std::move(first_branch));
        allof_wrapper.push_back(std::move(second_branch));
        schema.at("type").into(std::move(allof_wrapper));
        schema.rename("type", "allOf");
      }
    } else {
      schema.at("type").into(std::move(disjunctors));
      schema.rename("type", "anyOf");
    }
  }

private:
  mutable std::unordered_map<std::string, sourcemeta::core::JSON::TypeSet>
      keyword_instances_;
};
