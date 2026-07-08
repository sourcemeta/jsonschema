#ifndef SOURCEMETA_BLAZE_CANONICALIZER_RULE_H_
#define SOURCEMETA_BLAZE_CANONICALIZER_RULE_H_

class SchemaTransformRule {
public:
  SchemaTransformRule(const std::string_view name) : name_{name} {}

  virtual ~SchemaTransformRule() = default;

  SchemaTransformRule(const SchemaTransformRule &) = delete;
  SchemaTransformRule(SchemaTransformRule &&) = delete;
  auto operator=(const SchemaTransformRule &) -> SchemaTransformRule & = delete;
  auto operator=(SchemaTransformRule &&) -> SchemaTransformRule & = delete;

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

  [[nodiscard]] virtual auto condition(
      const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
      const sourcemeta::blaze::Vocabularies &vocabularies,
      const sourcemeta::blaze::SchemaFrame &frame,
      const sourcemeta::blaze::SchemaFrame::Location &location,
      const sourcemeta::blaze::SchemaWalker &walker,
      const sourcemeta::blaze::SchemaResolver &resolver) const -> bool = 0;

  virtual auto transform(sourcemeta::core::JSON &schema) const -> void = 0;

private:
  const std::string name_{};
};

#endif
