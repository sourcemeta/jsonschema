class DuplicateAnyOfBranches final : public SchemaTransformRule {
public:
  DuplicateAnyOfBranches()
      : SchemaTransformRule{
            "duplicate_anyof_branches",
            "Setting duplicate subschemas in `anyOf` is redundant, as it "
            "produces "
            "unnecessary additional validation that is guaranteed to not "
            "affect the validation result"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("anyOf") &&
                     schema.at("anyOf").is_array() &&
                     !schema.at("anyOf").unique());
    // TODO: Highlight which specific entries in `anyOf` are duplicated
    return APPLIES_TO_KEYWORDS("anyOf");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    this->index_mapping_.clear();
    const auto &original{schema.at("anyOf")};

    std::unordered_map<std::reference_wrapper<const JSON>, std::size_t,
                       HashJSON<std::reference_wrapper<const JSON>>,
                       EqualJSON<std::reference_wrapper<const JSON>>>
        seen;
    auto result{JSON::make_array()};

    for (std::size_t index = 0; index < original.size(); ++index) {
      const auto &value{original.at(index)};
      const auto match{seen.find(std::cref(value))};

      if (match == seen.end()) {
        this->index_mapping_[index] = seen.size();
        seen.emplace(std::cref(value), seen.size());
        result.push_back(value);
      } else {
        this->index_mapping_[index] = match->second;
      }
    }

    schema.assign("anyOf", std::move(result));
  }

  [[nodiscard]] auto rereference(const std::string &, const Pointer &,
                                 const Pointer &target,
                                 const Pointer &current) const
      -> Pointer override {
    const auto anyof_prefix{current.concat({"anyOf"})};
    const auto relative{target.resolve_from(anyof_prefix)};
    const auto old_index{relative.at(0).to_index()};
    const auto new_index{this->index_mapping_.at(old_index)};
    const Pointer old_prefix{anyof_prefix.concat({old_index})};
    const Pointer new_prefix{anyof_prefix.concat({new_index})};
    return target.rebase(old_prefix, new_prefix);
  }

private:
  mutable std::unordered_map<std::size_t, std::size_t> index_mapping_;
};
