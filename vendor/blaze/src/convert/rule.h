#ifndef SOURCEMETA_BLAZE_CONVERT_RULE_H_
#define SOURCEMETA_BLAZE_CONVERT_RULE_H_

class SchemaTransformRule {
public:
  SchemaTransformRule(const std::string_view name) : name_{name} {}

  virtual ~SchemaTransformRule() = default;

  SchemaTransformRule(const SchemaTransformRule &) = delete;
  SchemaTransformRule(SchemaTransformRule &&) = delete;
  auto operator=(const SchemaTransformRule &) -> SchemaTransformRule & = delete;
  auto operator=(SchemaTransformRule &&) -> SchemaTransformRule & = delete;

  /// Whether this rule writes outside the subschema it fires on. Such a rule
  /// is offered a subschema the ladder does not name, because what it is
  /// allowed to rewrite is its own business rather than the driver's
  using writes_outside_itself = std::false_type;

  [[nodiscard]] auto name() const noexcept -> std::string_view {
    return this->name_;
  }

  /// Where this rule moved a subschema, as pairs of positions relative to the
  /// subschema it fired on. The driver collects these so that a reference
  /// naming a moved position follows it, which is why a rule that relocates
  /// anything has to say so here rather than rewrite references itself
  using Relocation =
      std::pair<sourcemeta::core::Pointer, sourcemeta::core::Pointer>;

  [[nodiscard]] virtual auto relocations() const -> std::vector<Relocation> {
    return {};
  }

  /// Called once at the start of every pass, before anything is planned, so
  /// that a rule holding what it planned does not read back an answer about a
  /// document that has since changed
  virtual auto begin_pass() const -> void {}

  /// Whatever a rule needs the frame for, taken down while the frame still
  /// describes the document in front of it. A `SchemaFrame::Location` hands out
  /// views into the frame and the document, so reading one after an edit is a
  /// dangling read rather than a stale one, which is why anything frame-derived
  /// is captured here and nowhere else
  virtual auto plan(const sourcemeta::core::JSON &,
                    const sourcemeta::core::JSON &,
                    const sourcemeta::core::SchemaVocabularies &,
                    const sourcemeta::core::SchemaFrame &,
                    const sourcemeta::core::SchemaFrame::Location &,
                    const Site &, const sourcemeta::core::SchemaWalker &,
                    const sourcemeta::core::SchemaResolver &) const -> void {}

  [[nodiscard]] virtual auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const Site &site, const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const -> bool = 0;

  virtual auto transform(sourcemeta::core::JSON &schema, const Site &site) const
      -> void = 0;

private:
  const std::string name_{};
};

#endif
