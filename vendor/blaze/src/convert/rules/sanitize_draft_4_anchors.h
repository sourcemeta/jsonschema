/// Draft 4 lets an identifier carry any fragment, while Draft 6 restricts a
/// plain-name fragment to a narrow grammar. Renaming such an anchor means
/// moving every reference that names it, and a reference may name an anchor in
/// a resource other than the one it sits in, so this runs once over the whole
/// document rather than per resource. No single resource owns the problem:
/// whichever one is chosen to own it, the references that live outside it are
/// left pointing at a name that moved
class SanitizeDraft4Anchors final : public SchemaTransformRule {
public:
  // References to an anchor may sit anywhere, so this fires at the document
  // root and writes across it. Only Draft 4 subschemas are ever touched, which
  // is what the driver's own guard is there to protect
  using writes_outside_itself = std::true_type;

  SanitizeDraft4Anchors() : SchemaTransformRule{"sanitize_draft_4_anchors"} {};

  auto plan(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &, const Site &site,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> void override {
    if (!site.pointer.empty() || !schema.is_object()) {
      return;
    }

    this->moves_.clear();

    // Which anchors move, and what each becomes, settled before anything is
    // written so that a reference is matched against the name it still has
    std::map<sourcemeta::core::Pointer, Rename> renamed;
    plan_anchor_renames(schema, frame, renamed, this->moves_);
    if (renamed.empty()) {
      this->moves_.clear();
      return;
    }

    plan_reference_rewrites(frame, renamed, this->moves_);
  }

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &, const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &, const Site &site,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    // What was planned names positions across the whole document, so it is
    // written from the root and nowhere else
    return site.pointer.empty() && !this->moves_.empty();
  }

  auto transform(sourcemeta::core::JSON &schema, const Site &) const
      -> void override {
    for (const auto &[pointer, keyword, value] : this->moves_) {
      sourcemeta::core::get(schema, pointer).assign(keyword, value);
    }
  }

private:
  // The subschema to write into, the keyword to write, and the value. Holding
  // the parent rather than the keyword's own pointer keeps the write a plain
  // assignment
  using Move = std::tuple<sourcemeta::core::Pointer, std::string,
                          sourcemeta::core::JSON>;

  // The name a reference has to spell out to be following this anchor, and
  // the one it becomes
  struct Rename {
    std::string from;
    std::string to;
  };

  mutable std::vector<Move> moves_;

  static auto
  plan_anchor_renames(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaFrame &frame,
                      std::map<sourcemeta::core::Pointer, Rename> &renamed,
                      std::vector<Move> &moves) -> void {
    // Every anchor each resource already holds, so that a sanitised name never
    // lands on one that is taken
    std::map<std::string, std::set<std::string>> taken;

    // Framing reports one subschema once per location it holds, and an anchor
    // adds a second location at the same pointer. Keying by pointer is what
    // stops the repeat visit from seeing the name the first one reserved and
    // prefixing around it
    std::map<sourcemeta::core::Pointer, std::pair<std::string, std::string>>
        pending;
    frame.for_each_subschema(
        [&schema, &taken, &pending](
            const sourcemeta::core::SchemaFrame::Location &entry) -> void {
          // What dialect a subschema is read as is what decides whether its
          // identifier carries a free-form fragment, and asking the subschema
          // directly covers a root that never named itself
          if (base_dialect_position(entry.base_dialect) !=
              dialect_position(DRAFT_4_DIALECT)) {
            return;
          }

          std::string base{entry.base};

          auto pointer{sourcemeta::core::to_pointer(entry.pointer)};
          const auto &subschema{sourcemeta::core::get(schema, pointer)};
          if (!subschema.is_object()) {
            return;
          }

          const auto *identifier{subschema.try_at("id")};
          if (identifier == nullptr) {
            return;
          }

          const auto fragment{identifier_fragment(*identifier)};
          if (!fragment.has_value() || fragment.value().empty()) {
            return;
          }

          taken[base].insert(fragment.value());
          if (!is_draft_6_plain_name(fragment.value())) {
            pending.insert_or_assign(
                std::move(pointer),
                std::make_pair(std::move(base), fragment.value()));
          }
        });

    for (const auto &[pointer, entry] : pending) {
      const auto &[base, original] = entry;
      auto in_use{taken.at(base)};
      in_use.erase(original);
      auto sanitized{sanitize_anchor_with_policy(original, in_use,
                                                 draft_6_anchor_policy())};
      taken.at(base).insert(sanitized);

      const auto &written{
          sourcemeta::core::get(schema, pointer).at("id").to_string()};
      auto replacement{sourcemeta::core::JSON{
          std::string{without_fragment_as_written(written)} + "#" + sanitized}};
      moves.emplace_back(pointer, "id", std::move(replacement));
      renamed.emplace(pointer,
                      Rename{.from = original, .to = std::move(sanitized)});
    }
  }

  static auto plan_reference_rewrites(
      const sourcemeta::core::SchemaFrame &frame,
      const std::map<sourcemeta::core::Pointer, Rename> &renamed,
      std::vector<Move> &moves) -> void {
    frame.for_each_reference(
        [&frame, &renamed, &moves](
            const sourcemeta::core::SchemaReferenceType,
            const sourcemeta::core::WeakPointer &origin,
            const sourcemeta::core::SchemaFrame::Reference &reference) -> void {
          // A reference that names no fragment points at the resource itself
          // rather than at anything inside it
          if (!reference.fragment.has_value() ||
              reference.fragment.value().empty()) {
            return;
          }

          // Letting framing resolve the reference is what keeps the two sides
          // comparable. Matching the destination against a name built here
          // would miss a fragment that differs only in normalisation, such as
          // the case of a percent-encoded octet
          const auto target{frame.traverse(reference.destination)};
          if (!target.has_value()) {
            return;
          }

          const auto match{renamed.find(
              sourcemeta::core::to_pointer(target.value().get().pointer))};
          if (match == renamed.cend()) {
            return;
          }

          // Landing on a renamed subschema is not enough. A JSON Pointer
          // fragment reaches it by location and keeps working whatever the
          // anchor is called, while a Draft 4 anchor name may itself be shaped
          // like a pointer, so what decides is whether the reference spells
          // out the name that is moving
          if (reference.fragment.value() != match->second.from) {
            return;
          }

          auto replacement{sourcemeta::core::JSON{
              std::string{without_fragment_as_written(reference.original)} +
              "#" + match->second.to}};
          auto holder{sourcemeta::core::to_pointer(origin)};
          const auto keyword{holder.back().to_property()};
          holder.pop_back();
          moves.emplace_back(std::move(holder), keyword,
                             std::move(replacement));
        });
  }
};
