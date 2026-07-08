class DependentRequiredToAnyOf final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DependentRequiredToAnyOf()
      : SchemaTransformRule{"dependent_required_to_any_of"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Validation,
             Vocabularies::Known::JSON_Schema_2020_12_Validation}) &&
        schema.is_object());

    const auto *dependent_required{schema.try_at("dependentRequired")};
    ONLY_CONTINUE_IF(dependent_required && dependent_required->is_object() &&
                     !dependent_required->empty());

    ONLY_CONTINUE_IF(std::ranges::any_of(
        dependent_required->as_object(),
        [](const auto &entry) -> auto { return entry.second.is_array(); }));

    if (!vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_2020_12_Applicator})) {
      throw SchemaError(
          "Cannot canonicalise `dependentRequired` without the Applicator "
          "vocabulary");
    }

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto result_branches{sourcemeta::core::JSON::make_array()};

    std::vector<sourcemeta::core::JSON::String> processed;
    for (const auto &entry : schema.at("dependentRequired").as_object()) {
      if (!entry.second.is_array()) {
        continue;
      }

      auto required_all{sourcemeta::core::JSON::make_array()};
      required_all.push_back(sourcemeta::core::JSON{entry.first});
      for (const auto &dependent : entry.second.as_array()) {
        required_all.push_back(dependent);
      }

      auto absence_branch{sourcemeta::core::JSON::make_object()};
      absence_branch.assign("properties",
                            sourcemeta::core::JSON::make_object());
      absence_branch.at("properties")
          .assign(entry.first, sourcemeta::core::JSON{false});

      auto required_branch{sourcemeta::core::JSON::make_object()};
      required_branch.assign("type", sourcemeta::core::JSON{"object"});
      required_branch.assign("required", std::move(required_all));

      auto pair{sourcemeta::core::JSON::make_array()};
      pair.push_back(std::move(absence_branch));
      pair.push_back(std::move(required_branch));

      auto wrapper{sourcemeta::core::JSON::make_object()};
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
