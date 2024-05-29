#include <sourcemeta/hydra/http.h>
#include <sourcemeta/hydra/httpserver_logger.h>

#include <chrono>   // std::chrono::system_clock
#include <cstdint>  // std::uint8_t
#include <iostream> // std::cerr
#include <mutex>    // std::mutex, std::lock_guard
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#include <string_view> // std::string_view
#include <thread>      // std::this_thread
#include <utility>     // std::move

// Adapted from https://stackoverflow.com/a/58467162/1641422
static auto uuid() -> std::string {
  static std::random_device device;
  static std::mt19937 generator{device()};
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0,
                                  1, 0, 1, 0, 0, 0, 0, 0};
  std::uniform_int_distribution<decltype(digits)::size_type> distribution(0,
                                                                          15);
  std::string result;
  result.reserve(36);
  for (std::uint8_t index = 0; index < 16; index++) {
    if (dash[index]) {
      result += "-";
    }

    result += digits[distribution(generator)];
    result += digits[distribution(generator)];
  }

  return result;
}

namespace sourcemeta::hydra::http {

ServerLogger::ServerLogger() : ServerLogger{uuid()} {}

ServerLogger::ServerLogger(std::string &&id) : identifier{std::move(id)} {}

auto ServerLogger::id() const -> std::string_view { return this->identifier; }

auto ServerLogger::operator<<(std::string_view message) const -> void {
  // Otherwise we can get messed up output interleaved from multiple threads
  static std::mutex log_mutex;
  std::lock_guard<std::mutex> guard{log_mutex};
  std::cerr << "[" << to_gmt(std::chrono::system_clock::now()) << "] "
            << std::this_thread::get_id() << " (" << this->id() << ") "
            << message << "\n";
}

} // namespace sourcemeta::hydra::http
