#include <sourcemeta/blaze/test.h>

namespace sourcemeta::blaze {

auto TestSuite::run(const Callback &callback) -> Result {
  Result result{.total = this->tests.size(), .passed = 0};
  for (std::size_t index = 0; index < this->tests.size(); ++index) {
    const auto &test_case = this->tests[index];
    const auto actual{
        this->evaluator.validate(this->schema_fast, test_case.data)};
    callback(index + 1, this->tests.size(), test_case, actual);
    if (test_case.valid == actual) {
      result.passed += 1;
    }
  }

  return result;
}

} // namespace sourcemeta::blaze
