#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/core/jsonschema.h>

#include <utility> // std::move
#include <variant> // std::visit

static auto step_name(const sourcemeta::blaze::Instruction &instruction)
    -> std::string_view {
  return sourcemeta::blaze::InstructionNames
      [static_cast<std::underlying_type_t<sourcemeta::blaze::InstructionIndex>>(
          instruction.type)];
}

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(const sourcemeta::core::WeakPointer &base)
    : base_{base} {}

auto TraceOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto TraceOutput::end() const -> const_iterator { return this->output.end(); }

auto TraceOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto TraceOutput::cend() const -> const_iterator { return this->output.cend(); }

auto TraceOutput::operator()(
    const EvaluationType type, const bool result, const Instruction &step,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &) -> void {

  const auto short_step_name{step_name(step)};
  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};

  if (type == EvaluationType::Pre) {
    this->output.push_back({EntryType::Push, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location});
  } else if (result) {
    this->output.push_back({EntryType::Pass, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location});
  } else {
    this->output.push_back({EntryType::Fail, short_step_name, instance_location,
                            std::move(effective_evaluate_path),
                            step.keyword_location});
  }
}

} // namespace sourcemeta::blaze
