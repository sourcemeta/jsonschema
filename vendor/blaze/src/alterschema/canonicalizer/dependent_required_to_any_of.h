class DependentRequiredToAnyOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependentRequiredToAnyOf()
      : SchemaTransformRule{"dependent_required_to_any_of", ""} {};

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
            {Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object() && schema.defines("dependentRequired") &&
        schema.at("dependentRequired").is_object() &&
        !schema.at("dependentRequired").empty());

    ONLY_CONTINUE_IF(std::ranges::any_of(
        schema.at("dependentRequired").as_object(),
        [](const auto &entry) { return entry.second.is_array(); }));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto result_branches{JSON::make_array()};

    std::vector<JSON::String> processed;
    for (const auto &entry : schema.at("dependentRequired").as_object()) {
      if (!entry.second.is_array()) {
        continue;
      }

      auto required_all{JSON::make_array()};
      required_all.push_back(JSON{entry.first});
      for (const auto &dependent : entry.second.as_array()) {
        required_all.push_back(dependent);
      }

      auto not_required{JSON::make_object()};
      not_required.assign("type", JSON{"object"});
      not_required.assign("required", JSON::make_array());
      not_required.at("required").push_back(JSON{entry.first});
      auto not_branch{JSON::make_object()};
      not_branch.assign("not", std::move(not_required));

      auto required_branch{JSON::make_object()};
      required_branch.assign("type", JSON{"object"});
      required_branch.assign("required", std::move(required_all));

      auto pair{JSON::make_array()};
      pair.push_back(std::move(not_branch));
      pair.push_back(std::move(required_branch));

      auto wrapper{JSON::make_object()};
      wrapper.assign("anyOf", std::move(pair));
      result_branches.push_back(std::move(wrapper));

      processed.emplace_back(entry.first);
    }

    for (const auto &key : processed) {
      schema.at("dependentRequired").erase(key);
    }

    if (schema.at("dependentRequired").empty()) {
      schema.erase("dependentRequired");
    }

    if (schema.defines("allOf") && schema.at("allOf").is_array()) {
      for (auto &item : result_branches.as_array()) {
        schema.at("allOf").push_back(std::move(item));
      }
    } else {
      schema.assign("allOf", std::move(result_branches));
    }
  }
};
