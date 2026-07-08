class DisallowToArrayOfSchemas final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  DisallowToArrayOfSchemas()
      : SchemaTransformRule{"disallow_to_array_of_schemas"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
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

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    const auto &disallow{schema.at("disallow")};

    if (disallow.is_string()) {
      auto array{sourcemeta::core::JSON::make_array()};
      if (this->convert_to_schemas_) {
        array.push_back(type_string_to_schema(disallow.to_string()));
      } else {
        array.push_back(disallow);
      }
      schema.assign("disallow", std::move(array));
      return;
    }

    auto new_array{sourcemeta::core::JSON::make_array()};
    for (const auto &element : disallow.as_array()) {
      if (element.is_string() && this->convert_to_schemas_) {
        new_array.push_back(type_string_to_schema(element.to_string()));
      } else {
        new_array.push_back(element);
      }
    }
    schema.assign("disallow", std::move(new_array));
  }

  [[nodiscard]] auto rereference(const std::string_view,
                                 const sourcemeta::core::Pointer &,
                                 const sourcemeta::core::Pointer &target,
                                 const sourcemeta::core::Pointer &current) const
      -> std::optional<sourcemeta::core::Pointer> override {
    return target.rebase(
        current.concat("disallow"),
        current.concat(sourcemeta::core::Pointer{"disallow", 0}));
  }

private:
  static auto type_string_to_schema(const std::string &type_name)
      -> sourcemeta::core::JSON {
    if (type_name == "null") {
      auto result{sourcemeta::core::JSON::make_object()};
      auto values{sourcemeta::core::JSON::make_array()};
      values.push_back(sourcemeta::core::JSON{nullptr});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "boolean") {
      auto result{sourcemeta::core::JSON::make_object()};
      auto values{sourcemeta::core::JSON::make_array()};
      values.push_back(sourcemeta::core::JSON{false});
      values.push_back(sourcemeta::core::JSON{true});
      result.assign("enum", std::move(values));
      return result;
    }

    if (type_name == "any") {
      return sourcemeta::core::JSON::make_object();
    }

    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("type", sourcemeta::core::JSON{type_name});
    return result;
  }

  mutable bool convert_to_schemas_{true};
};
