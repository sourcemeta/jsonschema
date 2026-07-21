class UnknownTypeNames final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  UnknownTypeNames() : SchemaTransformRule{"unknown_type_names"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);

    // An unrecognised type name constrains nothing, so it is safe (and more
    // canonical) to drop it: a scalar unknown name leaves no constraint at all,
    // and an unknown name inside a union just does not contribute an
    // alternative. `parse_schema_type` yields an empty set for a name it does
    // not recognise
    if (type->is_string()) {
      return !parse_schema_type(*type).any();
    }

    if (type->is_array()) {
      for (const auto &entry : type->as_array()) {
        if (entry.is_string() && !parse_schema_type(entry).any()) {
          return true;
        }
      }
    }

    return false;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (schema.at("type").is_string()) {
      schema.erase("type");
      return;
    }

    auto recognised{sourcemeta::core::JSON::make_array()};
    for (const auto &entry : schema.at("type").as_array()) {
      if (entry.is_string() && !parse_schema_type(entry).any()) {
        continue;
      }

      recognised.push_back(entry);
    }

    if (recognised.empty()) {
      schema.erase("type");
    } else {
      schema.at("type").into(std::move(recognised));
    }
  }
};
