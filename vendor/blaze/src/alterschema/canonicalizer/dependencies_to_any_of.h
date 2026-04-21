class DependenciesToAnyOf final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DependenciesToAnyOf() : SchemaTransformRule{"dependencies_to_any_of", ""} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_7}) &&
        schema.is_object() && schema.defines("dependencies") &&
        schema.at("dependencies").is_object());

    ONLY_CONTINUE_IF(std::ranges::any_of(
        schema.at("dependencies").as_object(), [](const auto &entry) {
          return is_schema(entry.second) || entry.second.is_array();
        }));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto result_branches{JSON::make_array()};

    std::vector<JSON::String> processed;
    for (const auto &entry : schema.at("dependencies").as_object()) {
      if (is_schema(entry.second)) {
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
      } else if (entry.second.is_array()) {
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
      } else {
        continue;
      }

      processed.emplace_back(entry.first);
    }

    for (const auto &key : processed) {
      schema.at("dependencies").erase(key);
    }

    if (schema.at("dependencies").empty()) {
      schema.erase("dependencies");
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
