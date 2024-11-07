#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <utility> // std::move
#include <variant> // std::visit

#ifdef __clang__
#include <cxxabi.h> // abi::__cxa_demangle
#include <memory>   // std::free
static auto step_name(const sourcemeta::blaze::Template::value_type &step)
    -> std::string {
  return std::visit(
      [](const auto &value) {
        int status;
        std::string name{typeid(value).name()};
        char *demangled =
            abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status);
        if (demangled) {
          name = demangled;
          std::free(demangled);
        }

        return name;
      },
      step);
}
#else
static auto step_name(const sourcemeta::blaze::Template::value_type &)
    -> std::string {
  // TODO: Properly implement for GCC and MSVC
  return "????";
}
#endif

namespace sourcemeta::blaze {

TraceOutput::TraceOutput(const sourcemeta::jsontoolkit::WeakPointer &base)
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
    const EvaluationType type, const bool result,
    const Template::value_type &step,
    const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
    const sourcemeta::jsontoolkit::WeakPointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &) -> void {
  const std::string step_prefix{"sourcemeta::blaze::"};
  const auto full_step_name{step_name(step)};
  const auto short_step_name{full_step_name.starts_with(step_prefix)
                                 ? full_step_name.substr(step_prefix.size())
                                 : full_step_name};

  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};

  if (type == EvaluationType::Pre) {
    this->output.push_back({EntryType::Push, short_step_name, instance_location,
                            std::move(effective_evaluate_path)});
  } else if (result) {
    this->output.push_back({EntryType::Pass, short_step_name, instance_location,
                            std::move(effective_evaluate_path)});
  } else {
    this->output.push_back({EntryType::Fail, short_step_name, instance_location,
                            std::move(effective_evaluate_path)});
  }
}

} // namespace sourcemeta::blaze
