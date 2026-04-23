#ifndef SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_
#define SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_

#include <curl/curl.h>

#include <cctype>      // std::tolower
#include <cstddef>     // std::size_t
#include <map>         // std::map
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::jsonschema {

struct HTTPResponse {
  long status_code;
  std::map<std::string, std::string> headers;
  std::string body;
};

namespace detail {

static auto http_body_callback(char *data, std::size_t size, std::size_t count,
                               void *userdata) -> std::size_t {
  auto *body{static_cast<std::string *>(userdata)};
  const auto total{size * count};
  body->append(data, total);
  return total;
}

static auto http_header_callback(char *data, std::size_t size,
                                 std::size_t count, void *userdata)
    -> std::size_t {
  auto *headers{static_cast<std::map<std::string, std::string> *>(userdata)};
  const auto total{size * count};
  const std::string_view header{data, total};

  const auto separator{header.find(':')};
  if (separator == std::string_view::npos) {
    return total;
  }

  std::string name;
  name.reserve(separator);
  for (std::size_t index{0}; index < separator; ++index) {
    name.push_back(static_cast<char>(
        std::tolower(static_cast<unsigned char>(header[index]))));
  }

  auto value{header.substr(separator + 1)};
  while (!value.empty() && value.front() == ' ') {
    value.remove_prefix(1);
  }
  while (!value.empty() && (value.back() == '\r' || value.back() == '\n' ||
                            value.back() == ' ')) {
    value.remove_suffix(1);
  }

  headers->emplace(std::move(name), std::string{value});
  return total;
}

} // namespace detail

inline auto http_get(const std::string &url) -> HTTPResponse {
  auto *handle{curl_easy_init()};
  if (!handle) {
    throw std::runtime_error("Failed to initialize HTTP client");
  }

  HTTPResponse response{0, {}, {}};
  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, detail::http_body_callback);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response.body);
  curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION,
                   detail::http_header_callback);
  curl_easy_setopt(handle, CURLOPT_HEADERDATA, &response.headers);

  const auto result{curl_easy_perform(handle)};
  if (result == CURLE_OK) {
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response.status_code);
  }

  curl_easy_cleanup(handle);
  return response;
}

} // namespace sourcemeta::jsonschema

#endif
