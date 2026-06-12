#include "http.h"

#include <curl/curl.h> // curl_easy_*, curl_slist_*, curl_global_init

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

class CurlHandle {
public:
  CurlHandle() : handle_{curl_easy_init()} {}
  ~CurlHandle() {
    if (this->handle_) {
      curl_easy_cleanup(this->handle_);
    }
  }

  CurlHandle(const CurlHandle &) = delete;
  auto operator=(const CurlHandle &) -> CurlHandle & = delete;
  CurlHandle(CurlHandle &&) = delete;
  auto operator=(CurlHandle &&) -> CurlHandle & = delete;

  auto get() const -> CURL * { return this->handle_; }
  explicit operator bool() const { return this->handle_ != nullptr; }

private:
  CURL *handle_;
};

class CurlHeaderList {
public:
  CurlHeaderList() = default;
  ~CurlHeaderList() {
    if (this->list_) {
      curl_slist_free_all(this->list_);
    }
  }

  CurlHeaderList(const CurlHeaderList &) = delete;
  auto operator=(const CurlHeaderList &) -> CurlHeaderList & = delete;
  CurlHeaderList(CurlHeaderList &&) = delete;
  auto operator=(CurlHeaderList &&) -> CurlHeaderList & = delete;

  auto append(const std::string &line) -> void {
    auto *result{curl_slist_append(this->list_, line.c_str())};
    if (result) {
      this->list_ = result;
    }
  }

  auto get() const -> curl_slist * { return this->list_; }

private:
  curl_slist *list_{nullptr};
};

struct BodyContext {
  std::string *output;
  std::optional<std::size_t> maximum_size;
  bool maximum_size_exceeded{false};
};

auto body_callback(char *data, std::size_t size, std::size_t count,
                   void *user_data) -> std::size_t {
  auto *context{static_cast<BodyContext *>(user_data)};
  if (context->maximum_size.has_value() &&
      context->output->size() + (size * count) >
          context->maximum_size.value()) {
    context->maximum_size_exceeded = true;
    // Returning a smaller count than given aborts the transfer
    return 0;
  }

  context->output->append(data, size * count);
  return size * count;
}

auto header_callback(char *data, std::size_t size, std::size_t count,
                     void *output) -> std::size_t {
  sourcemeta::core::http_accumulate_header_line(
      *static_cast<std::string *>(output),
      std::string_view{data, size * count});
  return size * count;
}

} // namespace

namespace sourcemeta::jsonschema {

auto http_request(const HTTPRequest &request) -> HTTPResponse {
  static const CURLcode global_initialization{
      curl_global_init(CURL_GLOBAL_ALL)};
  if (global_initialization != CURLE_OK) {
    throw sourcemeta::core::HTTPError{
        request.method, std::string{request.url},
        curl_easy_strerror(global_initialization)};
  }

  const CurlHandle handle;
  if (!handle) {
    throw sourcemeta::core::HTTPError{request.method, std::string{request.url},
                                      "Failed to initialise the HTTP client"};
  }

  HTTPResponse response;
  const std::string url{request.url};
  curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(handle.get(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L);
  // Advertise and transparently decode all supported content encodings,
  // matching what the NSURLSession and WinHTTP backends do
  curl_easy_setopt(handle.get(), CURLOPT_ACCEPT_ENCODING, "");

  std::string raw_headers;
  BodyContext body_context{&response.body, request.maximum_response_size};
  curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, body_callback);
  curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &body_context);
  curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(handle.get(), CURLOPT_HEADERDATA, &raw_headers);

  CurlHeaderList header_list;
  for (const auto &[name, value] : request.headers) {
    std::string line{name};
    // The semicolon form is how cURL distinguishes a header with an
    // empty value from a header to suppress
    if (value.empty()) {
      line += ";";
    } else {
      line += ": ";
      line += value;
    }

    header_list.append(line);
  }

  if (request.body.has_value()) {
    std::string content_type_line{"Content-Type: "};
    content_type_line += request.body.value().content_type;
    header_list.append(content_type_line);
    curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDSIZE_LARGE,
                     static_cast<curl_off_t>(request.body.value().data.size()));
    curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS,
                     request.body.value().data.data());
  }

  if (header_list.get()) {
    curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, header_list.get());
  }

  const std::string method{
      sourcemeta::core::http_method_string(request.method)};
  if (request.method == sourcemeta::core::HTTPMethod::HEAD) {
    curl_easy_setopt(handle.get(), CURLOPT_NOBODY, 1L);
  } else if (request.method != sourcemeta::core::HTTPMethod::GET ||
             request.body.has_value()) {
    curl_easy_setopt(handle.get(), CURLOPT_CUSTOMREQUEST, method.c_str());
  }

  const auto code{curl_easy_perform(handle.get())};
  if (code != CURLE_OK) {
    if (body_context.maximum_size_exceeded) {
      throw sourcemeta::core::HTTPError{
          request.method, std::string{request.url},
          std::string{HTTP_RESPONSE_TOO_LARGE_MESSAGE}};
    }

    throw sourcemeta::core::HTTPError{request.method, std::string{request.url},
                                      curl_easy_strerror(code)};
  }

  long status_code{0};
  curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status_code);
  sourcemeta::core::http_parse_headers(raw_headers, response.headers);
  response.status = sourcemeta::core::http_status_from_code(
      static_cast<std::uint16_t>(status_code));
  return response;
}

} // namespace sourcemeta::jsonschema
