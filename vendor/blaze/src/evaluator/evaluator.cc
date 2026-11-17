#include <sourcemeta/blaze/evaluator.h>

namespace sourcemeta::blaze {

auto Evaluator::enter_entrypoint_resource(const Template &schema) -> void {
  this->dynamic_bindings.assign(schema.anchors.size(), 0);
  for (const auto &binding : schema.bindings[schema.target_bindings[0]]) {
    this->dynamic_bindings[binding.first] = binding.second;
  }
}

} // namespace sourcemeta::blaze
