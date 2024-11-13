#include <sourcemeta/hydra/crypto.h>
#include <sourcemeta/hydra/http.h>
#include <sourcemeta/hydra/httpserver_logger.h>

#include <chrono>      // std::chrono::system_clock
#include <iostream>    // std::cerr
#include <mutex>       // std::mutex, std::lock_guard
#include <string_view> // std::string_view
#include <thread>      // std::this_thread
#include <utility>     // std::move

namespace sourcemeta::hydra::http {

ServerLogger::ServerLogger() : ServerLogger{sourcemeta::hydra::uuid()} {}

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
