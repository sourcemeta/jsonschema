#include <sourcemeta/blaze/test.h>

#include <chrono>   // std::chrono::steady_clock
#include <optional> // std::nullopt
#include <utility>  // std::move
#include <variant>  // std::get, std::holds_alternative

namespace {

auto evaluate_test_case(sourcemeta::blaze::Evaluator &evaluator,
                        const sourcemeta::blaze::Template &schema,
                        const sourcemeta::blaze::TestCase &test_case)
    -> sourcemeta::blaze::TestOutcome {
  if (!test_case.rdf.has_value()) {
    const auto valid{evaluator.validate(schema, test_case.data)};
    return {.passed = test_case.valid == valid,
            .valid = valid,
            .rdf = std::nullopt,
            .rdf_error = std::nullopt};
  }

  auto promotion{sourcemeta::blaze::jsonld(evaluator, schema, test_case.data)};
  if (std::holds_alternative<sourcemeta::core::JSON>(promotion)) {
    auto expansion{std::get<sourcemeta::core::JSON>(std::move(promotion))};
    const auto passed{test_case.valid && expansion == test_case.rdf.value()};
    return {.passed = passed,
            .valid = true,
            .rdf = std::move(expansion),
            .rdf_error = std::nullopt};
  } else if (std::holds_alternative<sourcemeta::blaze::JSONLDResolutionError>(
                 promotion)) {
    return {.passed = false,
            .valid = true,
            .rdf = std::nullopt,
            .rdf_error = std::get<sourcemeta::blaze::JSONLDResolutionError>(
                std::move(promotion))};
  } else {
    return {.passed = false,
            .valid = false,
            .rdf = std::nullopt,
            .rdf_error = std::nullopt};
  }
}

} // namespace

namespace sourcemeta::blaze {

auto TestSuite::run(const Callback &callback) -> Result {
  const auto total{this->targets.size() * this->tests.size()};
  Result result{.total = total,
                .passed = 0,
                .start = std::chrono::steady_clock::now(),
                .end = {}};

  std::size_t step{0};
  for (std::size_t target_index = 0; target_index < this->targets.size();
       ++target_index) {
    const auto &target = this->targets[target_index];
    const auto &schema_fast = this->fast(target_index);
    for (const auto &test_case : this->tests) {
      const auto start{std::chrono::steady_clock::now()};
      const auto outcome{
          evaluate_test_case(this->evaluator, schema_fast, test_case)};
      const auto end{std::chrono::steady_clock::now()};
      step += 1;
      callback(target, step, total, test_case, outcome, start, end);
      if (outcome.passed) {
        result.passed += 1;
      }
    }
  }

  result.end = std::chrono::steady_clock::now();
  return result;
}

} // namespace sourcemeta::blaze
