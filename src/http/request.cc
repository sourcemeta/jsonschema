#include <sourcemeta/jsonschema/http.h>

#include <algorithm>        // std::copy
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::ostream_iterator
#include <span>             // std::span
#include <sstream>          // std::ostringstream
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <unordered_map>    // std::unordered_map
#include <utility>          // std::move

namespace sourcemeta::jsonschema::http {

ClientRequest::ClientRequest(std::string url) : stream{std::move(url)} {}

auto ClientRequest::method(const Method method) noexcept -> void {
  this->stream.method(method);
}

auto ClientRequest::method() const noexcept -> Method {
  return this->stream.method();
}

auto ClientRequest::capture(std::string header) -> void {
  this->capture_all_ = false;
  this->capture_.insert(std::move(header));
}

auto ClientRequest::capture(std::initializer_list<std::string> headers)
    -> void {
  this->capture_all_ = false;
  this->capture_.insert(headers);
}

auto ClientRequest::capture() -> void { this->capture_all_ = true; }

auto ClientRequest::header(std::string_view key, std::string_view value)
    -> void {
  this->stream.header(key, value);
}

auto ClientRequest::header(std::string_view key, int value) -> void {
  this->stream.header(key, value);
}

auto ClientRequest::url() const -> std::string_view {
  return this->stream.url();
}

auto ClientRequest::send(std::istream &body) -> ClientResponse {
  std::ostringstream output;
  this->stream.on_data(
      [&output](const Status, std::span<const std::uint8_t> buffer) noexcept {
        std::copy(buffer.begin(), buffer.end(),
                  std::ostream_iterator<char>(output));
      });

  std::unordered_map<std::string, std::string> headers;
  this->stream.on_header([&headers, this](const Status, std::string_view key,
                                          std::string_view value) noexcept {
    std::string header{key};
    if (this->capture_.contains(header) || this->capture_all_) {
      headers.insert_or_assign(std::move(header), std::string{value});
    }
  });

  this->stream.on_body([&body](const std::size_t bytes) {
    std::vector<std::uint8_t> result;
    while (result.size() < bytes &&
           body.peek() != std::istream::traits_type::eof()) {
      result.push_back(static_cast<decltype(result)::value_type>(body.get()));
    }

    return result;
  });

  return {this->stream.send(), std::move(headers), std::move(output)};
}

auto ClientRequest::send() -> ClientResponse {
  std::ostringstream output;
  this->stream.on_data(
      [&output](const Status, std::span<const std::uint8_t> buffer) noexcept {
        std::copy(buffer.begin(), buffer.end(),
                  std::ostream_iterator<char>(output));
      });

  std::unordered_map<std::string, std::string> headers;
  this->stream.on_header([&headers, this](const Status, std::string_view key,
                                          std::string_view value) noexcept {
    std::string header{key};
    if (this->capture_.contains(header) || this->capture_all_) {
      headers.insert_or_assign(std::move(header), std::string{value});
    }
  });

  return {this->stream.send(), std::move(headers), std::move(output)};
}

} // namespace sourcemeta::jsonschema::http
