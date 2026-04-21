class DisallowToArrayOfSchemas final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  DisallowToArrayOfSchemas()
      : SchemaTransformRule{"disallow_to_array_of_schemas", ""} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3}) &&
        schema.is_object() && schema.defines("disallow"));

    this->convert_to_schemas_ =
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_3);

    const auto &disallow{schema.at("disallow")};
    if (disallow.is_string()) {
      return true;
    }

    if (this->convert_to_schemas_) {
      ONLY_CONTINUE_IF(disallow.is_array());
      for (const auto &element : disallow.as_array()) {
        if (element.is_string()) {
          return true;
        }
      }
    }

    return false;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const auto &disallow{schema.at("disallow")};

    if (disallow.is_string()) {
      auto array{JSON::make_array()};
      if (this->convert_to_schemas_) {
        array.push_back(type_string_to_schema(disallow.to_string()));
      } else {
        array.push_back(disallow);
      }
      schema.assign("disallow", std::move(array));
      return;
    }

    auto new_array{JSON::make_array()};
    for (const auto &element : disallow.as_array()) {
      if (element.is_string() && this->convert_to_schemas_) {
        new_array.push_back(type_string_to_schema(element.to_string()));
      } else {
        new_array.push_back(element);
      }
    }
    schema.assign("disallow", std::move(new_array));
  }

  [[nodiscard]] auto rereference(const std::string_view, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    return target.rebase(current.concat({"disallow"}),
                         current.concat({"disallow", 0}));
  }

private:
  static auto type_string_to_schema(const std::string &type_name) -> JSON {
    if (type_name == "null") {
      auto result{JSON::make_object()};
      auto values{JSON::make_array()};
      values.push_back(JSON{nullptr});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "boolean") {
      auto result{JSON::make_object()};
      auto values{JSON::make_array()};
      values.push_back(JSON{false});
      values.push_back(JSON{true});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "any") {
      return JSON::make_object();
    }

    auto result{JSON::make_object()};
    result.assign("type", JSON{type_name});
    return result;
  }

  mutable bool convert_to_schemas_{true};
};
