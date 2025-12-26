#include <sourcemeta/blaze/test.h>

#include <chrono> // std::chrono::steady_clock

namespace sourcemeta::blaze {

auto TestSuite::run(const Callback &callback) -> Result {
  Result result{.total = this->tests.size(),
                .passed = 0,
                .start = std::chrono::steady_clock::now(),
                .end = {}};
  for (std::size_t index = 0; index < this->tests.size(); ++index) {
    const auto &test_case = this->tests[index];
    const auto start{std::chrono::steady_clock::now()};
    const auto actual{
        this->evaluator.validate(this->schema_fast, test_case.data)};
    const auto end{std::chrono::steady_clock::now()};
    callback(this->target, index + 1, this->tests.size(), test_case, actual,
             start, end);
    if (test_case.valid == actual) {
      result.passed += 1;
    }
  }

  result.end = std::chrono::steady_clock::now();
  return result;
}

} // namespace sourcemeta::blaze
