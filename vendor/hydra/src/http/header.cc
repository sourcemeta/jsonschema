#include <sourcemeta/hydra/http_header.h>
#include <sourcemeta/hydra/http_time.h>

#include <algorithm> // std::sort
#include <sstream>   // std::stringstream

namespace sourcemeta::hydra::http {

auto header_gmt(const std::string &value)
    -> std::chrono::system_clock::time_point {
  return from_gmt(value);
}

auto header_list(const std::string &value) -> std::vector<HeaderListElement> {
  std::stringstream stream{value};
  std::string token;
  std::vector<HeaderListElement> result;

  while (std::getline(stream, token, ',')) {
    const std::size_t start{token.find_first_not_of(" ")};
    const std::size_t end{token.find_last_not_of(" ")};
    if (start == std::string::npos || end == std::string::npos) {
      continue;
    }

    // See https://developer.mozilla.org/en-US/docs/Glossary/Quality_values
    const std::size_t value_start{token.find_first_of(";")};
    if (value_start != std::string::npos && token[value_start + 1] == 'q' &&
        token[value_start + 2] == '=') {
      result.emplace_back(token.substr(start, value_start - start),
                          std::stof(token.substr(value_start + 3)));
    } else {
      // No quality value is 1.0 by default
      result.emplace_back(token.substr(start, end - start + 1), 1.0f);
    }
  }

  // For convenient, automatically sort by the quality value
  std::sort(result.begin(), result.end(),
            [](const auto &left, const auto &right) {
              return left.second > right.second;
            });

  return result;
}

} // namespace sourcemeta::hydra::http
