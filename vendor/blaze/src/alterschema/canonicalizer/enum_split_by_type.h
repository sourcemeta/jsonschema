class EnumSplitByType final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EnumSplitByType()
      : SchemaTransformRule{
            "enum_split_by_type",
            "An `enum` whose values span more than one type is the disjunction "
            "of its single-type subsets, so it splits into a union of "
            "single-type enums"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &walker,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    const bool any_of_dialect{
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_7}) ||
        (vocabularies.contains(
             Vocabularies::Known::JSON_Schema_2019_09_Validation) &&
         vocabularies.contains(
             Vocabularies::Known::JSON_Schema_2019_09_Applicator)) ||
        (vocabularies.contains(
             Vocabularies::Known::JSON_Schema_2020_12_Validation) &&
         vocabularies.contains(
             Vocabularies::Known::JSON_Schema_2020_12_Applicator))};
    const bool type_union_dialect{
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_3})};
    ONLY_CONTINUE_IF((any_of_dialect || type_union_dialect) &&
                     schema.is_object());

    const auto *enumeration{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enumeration && enumeration->is_array() &&
                     !enumeration->empty());

    JSON::TypeSet kinds;
    for (const auto &value : enumeration->as_array()) {
      kinds.set(static_cast<std::size_t>(kind_of(value)));
    }
    ONLY_CONTINUE_IF(kinds.count() > 1);

    // The split moves only the `enum` values into branches and leaves every
    // sibling on this node, so it must not strand an assertion/applicator next
    // to the new union. Identity, metadata, and reference-into siblings ride
    // along, while validation siblings are isolated into an `allOf`/`extends`
    // by `enum_drop_redundant_validation` before this rule runs
    ONLY_CONTINUE_IF(
        wraps_single_constraint(schema, "enum", walker, vocabularies));

    this->use_any_of_ = any_of_dialect;
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto branches{JSON::make_array()};
    for (const auto &value : schema.at("enum").as_array()) {
      const auto kind{kind_of(value)};
      bool merged{false};
      for (auto &branch : branches.as_array()) {
        if (kind_of(branch.at("enum").at(0)) == kind) {
          branch.at("enum").push_back(value);
          merged = true;
          break;
        }
      }

      if (!merged) {
        auto values{JSON::make_array()};
        values.push_back(value);
        auto branch{JSON::make_object()};
        branch.assign("enum", std::move(values));
        branches.push_back(std::move(branch));
      }
    }

    schema.erase("enum");
    schema.assign(this->use_any_of_ ? "anyOf" : "type", std::move(branches));
  }

private:
  static auto kind_of(const sourcemeta::core::JSON &value)
      -> sourcemeta::core::JSON::Type {
    // Fold integers and reals into a single `number` kind, as an `enum` that
    // mixes them is still single-type
    const auto type{value.type()};
    return type == sourcemeta::core::JSON::Type::Integer
               ? sourcemeta::core::JSON::Type::Real
               : type;
  }

  static auto wraps_single_constraint(
      const sourcemeta::core::JSON &schema, const std::string_view keyword,
      const sourcemeta::blaze::SchemaWalker &walker,
      const sourcemeta::blaze::Vocabularies &vocabularies) -> bool {
    for (const auto &entry : schema.as_object()) {
      if (entry.first == keyword) {
        continue;
      }

      const auto type{walker(entry.first, vocabularies).type};
      if (type != SchemaKeywordType::Annotation &&
          type != SchemaKeywordType::Comment &&
          type != SchemaKeywordType::Other &&
          type != SchemaKeywordType::Unknown &&
          type != SchemaKeywordType::LocationMembers) {
        return false;
      }
    }

    return true;
  }

  mutable bool use_any_of_{false};
};
