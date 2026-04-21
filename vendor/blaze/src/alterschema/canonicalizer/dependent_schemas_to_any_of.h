class DependentSchemasToAnyOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependentSchemasToAnyOf()
      : SchemaTransformRule{"dependent_schemas_to_any_of", ""} {};

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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator}) &&
        schema.is_object() && schema.defines("dependentSchemas") &&
        schema.at("dependentSchemas").is_object() &&
        !schema.at("dependentSchemas").empty());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto result_branches{JSON::make_array()};

    for (const auto &entry : schema.at("dependentSchemas").as_object()) {
      auto not_required{JSON::make_object()};
      not_required.assign("type", JSON{"object"});
      not_required.assign("required", JSON::make_array());
      not_required.at("required").push_back(JSON{entry.first});
      auto not_branch{JSON::make_object()};
      not_branch.assign("not", std::move(not_required));

      auto required_obj{JSON::make_object()};
      required_obj.assign("type", JSON{"object"});
      required_obj.assign("required", JSON::make_array());
      required_obj.at("required").push_back(JSON{entry.first});

      auto all_of{JSON::make_array()};
      all_of.push_back(std::move(required_obj));
      all_of.push_back(entry.second);

      auto allof_branch{JSON::make_object()};
      allof_branch.assign("allOf", std::move(all_of));

      auto pair{JSON::make_array()};
      pair.push_back(std::move(not_branch));
      pair.push_back(std::move(allof_branch));

      auto wrapper{JSON::make_object()};
      wrapper.assign("anyOf", std::move(pair));
      result_branches.push_back(std::move(wrapper));
    }

    schema.erase("dependentSchemas");

    if (schema.defines("allOf") && schema.at("allOf").is_array()) {
      for (auto &item : result_branches.as_array()) {
        schema.at("allOf").push_back(std::move(item));
      }
    } else {
      schema.assign("allOf", std::move(result_branches));
    }
  }
};
