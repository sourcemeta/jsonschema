class InvalidExternalRef final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  InvalidExternalRef()
      : SchemaTransformRule{
            "invalid_external_ref",
            "External references must point to schemas that can be "
            "resolved"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &frame, const SchemaFrame::Location &location,
            const SchemaWalker &walker, const SchemaResolver &resolver) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(!frame.standalone());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Core,
         Vocabularies::Known::JSON_Schema_2019_09_Core,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_string());

    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    const auto reference_entry{
        frame.reference(SchemaReferenceType::Static, keyword_pointer)};
    ONLY_CONTINUE_IF(reference_entry.has_value());

    // If the destination exists in the frame, it is an internal reference
    ONLY_CONTINUE_IF(
        !frame.traverse(reference_entry->get().destination).has_value());

    const auto &reference_base{reference_entry->get().base};

    // Empty base with unresolvable destination is a local reference problem
    ONLY_CONTINUE_IF(!reference_base.empty());

    // Known official metaschemas are always resolvable
    ONLY_CONTINUE_IF(!is_known_schema(reference_base));

    // If the base exists in the frame, the reference is internal (e.g. an
    // embedded $id). A bad fragment on an internal base is handled by the
    // unknown_local_ref rule instead
    ONLY_CONTINUE_IF(!frame.traverse(reference_base).has_value());

    const auto &has_fragment{reference_entry->get().fragment.has_value()};
    const JSON::String base_key{reference_base};

    // Check the resolver cache to avoid redundant lookups
    const auto cached{this->resolver_cache_.find(base_key)};
    if (cached != this->resolver_cache_.end()) {
      if (!cached->second.has_value()) {
        return APPLIES_TO_KEYWORDS(KEYWORD);
      }

      if (has_fragment) {
        return this->is_fragment_invalid(reference_entry->get(), cached->second,
                                         base_key, walker, resolver, location)
                   ? APPLIES_TO_KEYWORDS(KEYWORD)
                   : false;
      }

      return false;
    }

    auto remote{resolver(reference_base)};
    const auto &[entry,
                 _]{this->resolver_cache_.emplace(base_key, std::move(remote))};
    if (!entry->second.has_value()) {
      return APPLIES_TO_KEYWORDS(KEYWORD);
    }

    if (has_fragment) {
      return this->is_fragment_invalid(reference_entry->get(), entry->second,
                                       base_key, walker, resolver, location)
                 ? APPLIES_TO_KEYWORDS(KEYWORD)
                 : false;
    }

    return false;
  }

private:
  static inline const std::string KEYWORD{"$ref"};
  mutable std::unordered_map<JSON::String, std::optional<JSON>> resolver_cache_;
  mutable std::unordered_map<JSON::String, std::unique_ptr<SchemaFrame>>
      frame_cache_;

  [[nodiscard]] auto
  is_fragment_invalid(const SchemaFrame::ReferencesEntry &reference_entry,
                      const std::optional<JSON> &remote,
                      const JSON::String &base_key, const SchemaWalker &walker,
                      const SchemaResolver &resolver,
                      const SchemaFrame::Location &location) const -> bool {
    auto frame_iterator{this->frame_cache_.find(base_key)};
    if (frame_iterator == this->frame_cache_.end()) {
      auto remote_frame{
          std::make_unique<SchemaFrame>(SchemaFrame::Mode::Locations)};
      remote_frame->analyse(remote.value(), walker, resolver, location.dialect,
                            base_key);
      frame_iterator =
          this->frame_cache_.emplace(base_key, std::move(remote_frame)).first;
    }

    return !frame_iterator->second->traverse(reference_entry.destination)
                .has_value();
  }
};
