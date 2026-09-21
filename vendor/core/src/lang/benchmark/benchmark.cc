#include <sourcemeta/core/benchmark.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/process.h>

#include <algorithm>   // std::max, std::min
#include <array>       // std::array
#include <cmath>       // std::llround
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <functional>  // std::less
#include <iomanip>     // std::setprecision, std::fixed
#include <iostream>    // std::cout, std::cerr
#include <ostream>     // std::ostream
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::thread::hardware_concurrency
#include <utility>     // std::move
#include <vector>      // std::vector

namespace {
using namespace std::string_view_literals;

constexpr auto BENCHMARK_HASH_VERSION{
    sourcemeta::core::JSON::Object::hash("version"sv)};
constexpr auto BENCHMARK_HASH_CONTEXT{
    sourcemeta::core::JSON::Object::hash("context"sv)};
constexpr auto BENCHMARK_HASH_CORES{
    sourcemeta::core::JSON::Object::hash("cores"sv)};
constexpr auto BENCHMARK_HASH_BENCHMARKS{
    sourcemeta::core::JSON::Object::hash("benchmarks"sv)};
constexpr auto BENCHMARK_HASH_NAME{
    sourcemeta::core::JSON::Object::hash("name"sv)};
constexpr auto BENCHMARK_HASH_ITERATIONS{
    sourcemeta::core::JSON::Object::hash("iterations"sv)};
constexpr auto BENCHMARK_HASH_REPETITIONS{
    sourcemeta::core::JSON::Object::hash("repetitions"sv)};
constexpr auto BENCHMARK_HASH_REAL_TIME{
    sourcemeta::core::JSON::Object::hash("realTime"sv)};
constexpr auto BENCHMARK_HASH_CPU_TIME{
    sourcemeta::core::JSON::Object::hash("cpuTime"sv)};
constexpr auto BENCHMARK_HASH_COLD_TIME{
    sourcemeta::core::JSON::Object::hash("coldTime"sv)};

// What the shape of this document is, so that a reader given one written by
// some other version has something to refuse on rather than a surprise
constexpr std::int64_t FORMAT_VERSION{1};

struct RegisteredBenchmark {
  std::string name;
  std::string_view file;
  int line;
  std::function<void(sourcemeta::core::BenchmarkState &)> body;
};

struct Measurement {
  std::string name;
  std::uint64_t iterations;
  double real_time;
  std::optional<double> cpu_time;
  // What the very first execution cost, before anything the benchmark touches
  // had a chance to warm up. A wide gap against the reported time is the sign
  // of a cache, a lazy initialisation, or an allocator that settles
  double cold_time;
};

auto base_name(const std::string_view path) -> std::string {
  return std::filesystem::path{path}.filename().string();
}

auto registry() -> std::vector<RegisteredBenchmark> & {
  static std::vector<RegisteredBenchmark> benchmarks;
  return benchmarks;
}

// How long one measurement is given before the amount of work it covers is
// taken as enough to time reliably
constexpr std::chrono::nanoseconds MINIMUM_RUN_TIME{170'000'000};

// How many times that work is then measured. A single window can be lost to
// whatever else the machine decided to do while it was open, and no number of
// iterations inside one window makes up for that. Measuring the same work
// several times over does, and the fastest of those is the one least
// disturbed, as interference only ever makes code slower. Together these cost
// about what a single longer measurement used to
constexpr std::size_t REPETITIONS{3};

// A run that cannot reach the target time within this many iterations is
// reported as it stands rather than grown further
constexpr std::uint64_t MAXIMUM_ITERATIONS{1'000'000'000'000};

constexpr std::uint64_t INITIAL_ITERATIONS{1};

// Aim beyond the target rather than exactly at it, so that a run which lands
// just short does not need another attempt
constexpr double OVERSHOOT{1.4};

// A run too short to extrapolate from grows by a flat factor instead, as
// scaling from a measurement dominated by noise overshoots wildly
constexpr double BLIND_GROWTH{10.0};
constexpr double BLIND_THRESHOLD{0.1};

auto next_iterations(const std::uint64_t current,
                     const std::chrono::nanoseconds elapsed) -> std::uint64_t {
  const auto target{static_cast<double>(MINIMUM_RUN_TIME.count())};
  const auto measured{std::max(static_cast<double>(elapsed.count()), 1.0)};
  const auto multiplier{(measured / target) > BLIND_THRESHOLD
                            ? (target * OVERSHOOT / measured)
                            : BLIND_GROWTH};
  const auto grown{
      std::llround(std::max(multiplier * static_cast<double>(current),
                            static_cast<double>(current) + 1.0))};
  return std::min(static_cast<std::uint64_t>(grown), MAXIMUM_ITERATIONS);
}

auto measure(const RegisteredBenchmark &benchmark)
    -> std::optional<Measurement> {
  auto iterations{INITIAL_ITERATIONS};
  std::optional<double> cold{std::nullopt};
  std::optional<double> real{std::nullopt};
  std::optional<double> cpu{std::nullopt};
  std::size_t repetitions{0};
  auto settled{false};

  while (true) {
    sourcemeta::core::BenchmarkState state{iterations};
    benchmark.body(state);

    if (!state.measured()) {
      return std::nullopt;
    }

    const auto elapsed{state.real_time()};
    if (!cold.has_value()) {
      cold = static_cast<double>(elapsed.count()) /
             static_cast<double>(iterations);
    }

    // Growing stops for good once the work is large enough to time, as a
    // later repetition that comes in faster is the point of repeating rather
    // than a sign that there is too little work to measure
    if (!settled) {
      settled = elapsed >= MINIMUM_RUN_TIME || iterations >= MAXIMUM_ITERATIONS;
      if (!settled) {
        iterations = next_iterations(iterations, elapsed);
        continue;
      }
    }

    const auto count{static_cast<double>(iterations)};
    const auto candidate{static_cast<double>(elapsed.count()) / count};
    if (!real.has_value() || candidate < real.value()) {
      real = candidate;
      cpu = state.cpu_time().has_value()
                ? std::optional<double>{static_cast<double>(
                                            state.cpu_time().value().count()) /
                                        count}
                : std::nullopt;
    }

    repetitions += 1;
    if (repetitions >= REPETITIONS) {
      return Measurement{.name = benchmark.name,
                         .iterations = iterations,
                         .real_time = real.value(),
                         .cpu_time = cpu,
                         .cold_time = cold.value()};
    }
  }
}

auto print_usage(const std::string_view program) -> void {
  std::cout << "Usage: " << std::filesystem::path{program}.stem().string()
            << " [options]\n\n"
            << "Run the registered benchmarks.\n\n"
            << "Options:\n"
            << "  -f, --filter <text>  Only run benchmarks whose name "
               "contains <text>\n"
            << "  -o, --output <path>  Also write the results as JSON to "
               "<path>\n"
            << "  -h, --help           Show this message\n";
}

auto print_environment() -> void {
  const auto cores{std::thread::hardware_concurrency()};
  if (cores > 0) {
    std::cout << "Running on " << cores << " processors\n";
  }
}

// A benchmark that takes seconds per iteration and one that takes nanoseconds
// both have to line up in the same table, so the unit follows the magnitude
auto format_duration(const double nanoseconds) -> std::string {
  struct Unit {
    double factor;
    std::string_view suffix;
  };

  static constexpr std::array<Unit, 4> UNITS{
      {{.factor = 1.0, .suffix = "ns"},
       {.factor = 1'000.0, .suffix = "us"},
       {.factor = 1'000'000.0, .suffix = "ms"},
       {.factor = 1'000'000'000.0, .suffix = "s"}}};

  auto chosen{UNITS.front()};
  for (const auto &unit : UNITS) {
    if (nanoseconds >= unit.factor) {
      chosen = unit;
    }
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << (nanoseconds / chosen.factor)
         << " " << chosen.suffix;
  return stream.str();
}

auto print_measurement(const Measurement &measurement) -> void {
  std::cout << measurement.name << "\n"
            << "  time: " << format_duration(measurement.real_time);
  if (measurement.cpu_time.has_value()) {
    std::cout << ", cpu: " << format_duration(measurement.cpu_time.value());
  }

  std::cout << ", first: " << format_duration(measurement.cold_time)
            << ", iterations: " << measurement.iterations << "\n";
}

// Every duration is nanoseconds for one iteration, and the real one is the
// fastest of the repetitions rather than an average over them
auto to_json(const std::vector<Measurement> &measurements)
    -> sourcemeta::core::JSON {
  auto context{sourcemeta::core::JSON::make_object()};
  const auto cores{std::thread::hardware_concurrency()};
  if (cores > 0) {
    context.assign_assume_new(
        "cores", sourcemeta::core::JSON{static_cast<std::int64_t>(cores)},
        BENCHMARK_HASH_CORES);
  }

  auto entries{sourcemeta::core::JSON::make_array()};
  for (const auto &measurement : measurements) {
    auto entry{sourcemeta::core::JSON::make_object()};
    entry.assign_assume_new("name", sourcemeta::core::JSON{measurement.name},
                            BENCHMARK_HASH_NAME);
    entry.assign_assume_new("iterations",
                            sourcemeta::core::JSON{static_cast<std::int64_t>(
                                measurement.iterations)},
                            BENCHMARK_HASH_ITERATIONS);
    entry.assign_assume_new(
        "repetitions",
        sourcemeta::core::JSON{static_cast<std::int64_t>(REPETITIONS)},
        BENCHMARK_HASH_REPETITIONS);
    entry.assign_assume_new("realTime",
                            sourcemeta::core::JSON{measurement.real_time},
                            BENCHMARK_HASH_REAL_TIME);

    // A platform that cannot account for processor time says so, rather than
    // standing the elapsed time in for it, which would read as a benchmark
    // that spent every moment of it computing
    entry.assign_assume_new(
        "cpuTime",
        measurement.cpu_time.has_value()
            ? sourcemeta::core::JSON{measurement.cpu_time.value()}
            : sourcemeta::core::JSON{nullptr},
        BENCHMARK_HASH_CPU_TIME);

    entry.assign_assume_new("coldTime",
                            sourcemeta::core::JSON{measurement.cold_time},
                            BENCHMARK_HASH_COLD_TIME);
    entries.push_back(std::move(entry));
  }

  auto document{sourcemeta::core::JSON::make_object()};
  document.assign_assume_new("version", sourcemeta::core::JSON{FORMAT_VERSION},
                             BENCHMARK_HASH_VERSION);
  document.assign_assume_new("context", std::move(context),
                             BENCHMARK_HASH_CONTEXT);
  document.assign_assume_new("benchmarks", std::move(entries),
                             BENCHMARK_HASH_BENCHMARKS);
  return document;
}

} // namespace

namespace sourcemeta::core {

BenchmarkState::BenchmarkState(const std::uint64_t iterations)
    : iterations_{iterations} {}

auto BenchmarkState::iterations() const -> std::uint64_t {
  return this->iterations_;
}

auto BenchmarkState::measured() const -> bool { return this->finished_; }

auto BenchmarkState::real_time() const -> std::chrono::nanoseconds {
  return this->real_elapsed_;
}

auto BenchmarkState::cpu_time() const
    -> std::optional<std::chrono::nanoseconds> {
  return this->cpu_elapsed_;
}

auto BenchmarkState::start() -> void {
  this->started_ = true;
  this->cpu_start_ = process_usage().cpu_time;
  this->real_start_ = std::chrono::steady_clock::now();
}

auto BenchmarkState::finish() -> void {
  if (this->finished_) {
    return;
  }

  const auto real_end{std::chrono::steady_clock::now()};
  const auto cpu_end{process_usage().cpu_time};
  this->real_elapsed_ = real_end - this->real_start_;
  if (this->cpu_start_.has_value() && cpu_end.has_value()) {
    this->cpu_elapsed_ = cpu_end.value() - this->cpu_start_.value();
  }

  this->finished_ = true;
}

#if !defined(__clang__) && !defined(__GNUC__)
auto benchmark_use_char_pointer(const volatile char *) -> void {}
#endif

auto benchmark_register(std::string_view name, std::string_view file, int line,
                        std::function<void(BenchmarkState &)> body) -> int {
  registry().push_back({.name = std::string{name},
                        .file = file,
                        .line = line,
                        .body = std::move(body)});
  return 0;
}

auto benchmark_run(int argc, char **argv) -> int {
  Options options;
  options.option("filter", {"f"});
  options.option("output", {"o"});
  options.flag("help", {"h"});

  try {
    options.parse(argc, argv);
  } catch (const OptionsError &) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (options.contains("help")) {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  std::string_view needle;
  if (options.contains("filter") && !options.at("filter").empty()) {
    needle = options.at("filter").front();
  }

  std::vector<const RegisteredBenchmark *> selected;
  for (const auto &entry : registry()) {
    if (needle.empty() || entry.name.contains(needle)) {
      selected.push_back(&entry);
    }
  }

  // An unfiltered run that selects nothing means registration is broken, which
  // must not pass as a successful run. A filter that matches nothing is instead
  // an ordinary thing to ask for interactively
  if (selected.empty() && needle.empty()) {
    std::cerr << "error: No benchmarks were registered\n";
    return EXIT_FAILURE;
  }

  print_environment();

  std::vector<Measurement> measurements;
  measurements.reserve(selected.size());
  for (const auto *entry : selected) {
    auto measurement{measure(*entry)};
    if (!measurement.has_value()) {
      std::cerr << "error: The benchmark did not iterate over its state\n"
                << "  at benchmark " << entry->name << "\n"
                << "  at source location " << base_name(entry->file) << ":"
                << entry->line << "\n";
      return EXIT_FAILURE;
    }

    print_measurement(measurement.value());
    measurements.push_back(std::move(measurement).value());
  }

  if (options.contains("output") && !options.at("output").empty()) {
    const std::filesystem::path destination{options.at("output").front()};
    const auto document{to_json(measurements)};

    try {
      write_file(destination, [&document](std::ostream &stream) {
        prettify(document, stream);
        stream << "\n";
      });
    } catch (const std::exception &error) {
      std::cerr << "error: " << error.what() << "\n"
                << "  at file path " << destination.string() << "\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

} // namespace sourcemeta::core
