class NonApplicableTypeSpecificKeywords final : public SchemaTransformRule {
public:
  NonApplicableTypeSpecificKeywords()
      : SchemaTransformRule{"non_applicable_type_specific_keywords",
                            "Avoid keywords that don't apply to the current "
                            "explicitly declared type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    if (!schema.is_object() || !schema.defines("type") ||
        !contains_any(vocabularies,
                      {"https://json-schema.org/draft/2020-12/vocab/validation",
                       "https://json-schema.org/draft/2019-09/vocab/validation",
                       "http://json-schema.org/draft-07/schema#",
                       "http://json-schema.org/draft-06/schema#",
                       "http://json-schema.org/draft-04/schema#",
                       "http://json-schema.org/draft-03/schema#",
                       "http://json-schema.org/draft-02/schema#",
                       "http://json-schema.org/draft-02/hyper-schema#",
                       "http://json-schema.org/draft-01/schema#",
                       "http://json-schema.org/draft-01/hyper-schema#",
                       "http://json-schema.org/draft-00/schema#",
                       "http://json-schema.org/draft-00/hyper-schema#"})) {
      return false;
    }

    std::set<JSON::Type> current_types;
    if (schema.at("type").is_string()) {
      this->emplace_type(schema.at("type").to_string(), current_types);
    } else if (schema.at("type").is_array()) {
      for (const auto &entry : schema.at("type").as_array()) {
        if (entry.is_string()) {
          this->emplace_type(entry.to_string(), current_types);
        }
      }
    }

    // This means that the schema has no explicit type declaration,
    // so we cannot remove anything from it. At least not based on types
    if (current_types.empty()) {
      return false;
    }

    this->blacklist.clear();
    for (const auto &entry : schema.as_object()) {
      const auto metadata{walker(entry.first, vocabularies)};

      // The keyword applies to any type, so it cannot be removed
      if (metadata.instances.empty()) {
        continue;
      }

      // If none of the types that the keyword applies to is a valid
      // type for the current schema, then by definition we can remove it
      if (std::ranges::none_of(metadata.instances,
                               [&current_types](const auto keyword_type) {
                                 return current_types.contains(keyword_type);
                               })) {
        this->blacklist.emplace_back(entry.first);
      }
    }

    if (this->blacklist.empty()) {
      return false;
    }

    // Print the offending keyword
    std::ostringstream message;
    for (const auto &entry : this->blacklist) {
      message << "- " << entry << "\n";
    }

    return message.str();
  }

  auto transform(JSON &schema) const -> void override {
    schema.erase_keys(this->blacklist.cbegin(), this->blacklist.cend());
  }

private:
  mutable std::vector<JSON::String> blacklist;

  template <typename T>
  auto emplace_type(const JSON::String &type, T &container) const -> void {
    if (type == "null") {
      container.emplace(JSON::Type::Null);
    } else if (type == "boolean") {
      container.emplace(JSON::Type::Boolean);
    } else if (type == "object") {
      container.emplace(JSON::Type::Object);
    } else if (type == "array") {
      container.emplace(JSON::Type::Array);
    } else if (type == "number") {
      container.emplace(JSON::Type::Integer);
      container.emplace(JSON::Type::Real);
    } else if (type == "integer") {
      container.emplace(JSON::Type::Integer);
    } else if (type == "string") {
      container.emplace(JSON::Type::String);
    }
  }
};
