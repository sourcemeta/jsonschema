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

  /// A method to optionally fix any reference location that was affected by the
  /// transformation
  [[nodiscard]] virtual auto
  rereference(const std::string_view, const sourcemeta::core::Pointer &,
              const sourcemeta::core::Pointer &,
              const sourcemeta::core::Pointer &) const
      -> std::optional<sourcemeta::core::Pointer> {
    return std::nullopt;
  }

  [[nodiscard]] virtual auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &root,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver) const -> bool = 0;

  virtual auto transform(sourcemeta::core::JSON &schema) const -> void = 0;

  /// The schemas the frame knows about under the one being transformed, as the
  /// driver saw them just before the transform ran. A transform has no frame of
  /// its own, and the ladder's marker only ever sits on a schema, so this is
  /// how a rule clears markers without walking into the caller's own data
  auto prepare(const sourcemeta::core::SchemaFrame &frame,
               const sourcemeta::core::SchemaFrame::Location &location)
      -> void {
    this->subschemas_ = subschema_pointers_under(frame, location);
  }

  [[nodiscard]] auto subschemas() const noexcept
      -> const std::vector<sourcemeta::core::Pointer> & {
    return this->subschemas_;
  }

private:
  const std::string name_{};
  std::vector<sourcemeta::core::Pointer> subschemas_;
};

#endif
