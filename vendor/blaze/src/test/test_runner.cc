#include <sourcemeta/blaze/test.h>

#include <chrono> // std::chrono::steady_clock

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
    const auto &schema_fast = this->schemas_fast[target_index];
    for (const auto &test_case : this->tests) {
      const auto start{std::chrono::steady_clock::now()};
      const auto actual{this->evaluator.validate(schema_fast, test_case.data)};
      const auto end{std::chrono::steady_clock::now()};
      step += 1;
      callback(target, step, total, test_case, actual, start, end);
      if (test_case.valid == actual) {
        result.passed += 1;
      }
    }
  }

  result.end = std::chrono::steady_clock::now();
  return result;
}

} // namespace sourcemeta::blaze
