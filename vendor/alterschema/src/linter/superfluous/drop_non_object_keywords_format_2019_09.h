class DropNonObjectKeywordsFormat_2019_09 final : public Rule {
public:
  DropNonObjectKeywordsFormat_2019_09()
      : Rule{"drop_non_object_keywords_format_2019_09",
             "Keywords that don't apply to objects will never match if the "
             "instance is guaranteed to be an object"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/format") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"format"};
};
