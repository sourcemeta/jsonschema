#ifndef SOURCEMETA_HYDRA_HTTPCLIENT_STREAM_H
#define SOURCEMETA_HYDRA_HTTPCLIENT_STREAM_H

#ifndef SOURCEMETA_HYDRA_HTTPCLIENT_EXPORT
#include <sourcemeta/hydra/httpclient_export.h>
#endif

#include <sourcemeta/hydra/http.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <future>      // std::future
#include <memory>      // std::unique_ptr
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::hydra::http {

/// @ingroup httpclient
/// This class is used to perform a streaming HTTP request.
class SOURCEMETA_HYDRA_HTTPCLIENT_EXPORT ClientStream {
public:
  /// Construct a streaming HTTP request to a given URL. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// request.method(sourcemeta::hydra::http::Method::GET);
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.on_header([](const sourcemeta::hydra::http::Status status,
  ///                      std::string_view key,
  ///                      std::string_view value) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Print the incoming headers
  ///   std::cout << key << " -> " << value << "\n";
  /// });
  ///
  /// request.send().wait();
  /// ```
  ClientStream(std::string url);

  /// Move an instance of this class. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <utility>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// sourcemeta::hydra::http::ClientStream new_request{std::move(request)};
  /// ```
  ClientStream(ClientStream &&other) noexcept;

  /// Move an instance of this class. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <utility>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// sourcemeta::hydra::http::ClientStream new_request = std::move(request);
  /// ```
  auto operator=(ClientStream &&other) noexcept -> ClientStream &;

  // While technically possible, copy semantics are not very useful here
  // Also, not worth documenting these.
#if !defined(DOXYGEN)
  ClientStream(const ClientStream &other) = delete;
  auto operator=(const ClientStream &other) -> ClientStream & = delete;
#endif

  /// Destruct an instance of this class.
  ~ClientStream();

  /// Specify the HTTP method to use for the request. If not set, it defauls to
  /// `GET`. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  ///
  /// // Send a POST request
  /// request.method(sourcemeta::hydra::http::Method::POST);
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto method(const Method method) noexcept -> void;

  /// Retrieve the HTTP method that the request will be sent with. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// request.method(sourcemeta::hydra::http::Method::HEAD);
  /// assert(request.method() == sourcemeta::hydra::http::Method::HEAD);
  /// ```
  auto method() const noexcept -> Method;

  /// Set an HTTP request header. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// request.header("X-Send-With", "Hydra");
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto header(std::string_view key, std::string_view value) -> void;

  /// Set an HTTP request header whose value is an integer. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// request.header("X-Favourite-Number", 3);
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto header(std::string_view key, int value) -> void;

  /// Retrieve the URL that the request will be sent to. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// std::cout << request.url() << "\n";
  /// ```
  auto url() const -> std::string_view;

  /// Perform the streaming HTTP request, resolving the response status code.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  /// request.method(sourcemeta::hydra::http::Method::GET);
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.on_header([](const sourcemeta::hydra::http::Status status,
  ///                      std::string_view key,
  ///                      std::string_view value) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Print the incoming headers
  ///   std::cout << key << " -> " << value << "\n";
  /// });
  ///
  /// auto status{request.send().get()};
  /// assert(status == sourcemeta::hydra::http::Status::OK);
  auto send() -> std::future<Status>;

  using DataCallback =
      std::function<void(const Status, std::span<const std::uint8_t>)>;
  using HeaderCallback =
      std::function<void(const Status, std::string_view, std::string_view)>;
  using BodyCallback =
      std::function<std::vector<std::uint8_t>(const std::size_t)>;

  /// Set a function that gets called every time there is new data to process.
  /// The callback gets passed the response status code and a buffer. Make sure
  /// your callback does not throw exceptions, as it may result in undefined
  /// behavior. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  ///
  /// request.on_data([](const sourcemeta::hydra::http::Status status,
  ///                    std::span<const std::uint8_t> buffer) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Copy to standard output
  ///   for (const auto byte : buffer) {
  ///     std::cout << static_cast<char>(byte);
  ///   }
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto on_data(DataCallback callback) noexcept -> void;

  /// Set a function that gets called every time there is a new response header
  /// to process. The callback gets passed the response status code, and the
  /// header key and value. Make sure your callback does not throw exceptions,
  /// as it may result in undefined behavior. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  ///
  /// request.on_header([](const sourcemeta::hydra::http::Status status,
  ///                      std::string_view key,
  ///                      std::string_view value) noexcept {
  ///   std::cerr << "Code: " << status << "\n";
  ///
  ///   // Print the incoming headers
  ///   std::cout << key << " -> " << value << "\n";
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto on_header(HeaderCallback callback) noexcept -> void;

  /// Set a function that gets called (potentially multiple times) to pass a
  /// request body. The callback gets passed the number of bytes to read, and
  /// its expected to return an array of bytes to pass to the request. If the
  /// number of returned bytes is less than the bytes argument, then the body is
  /// assumed to have ended.
  ///
  /// Keep in mind that passing a request body in this way will result in
  /// chunked encoding. See
  /// https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Transfer-Encoding.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpclient.h>
  /// #include <sstream>
  ///
  /// sourcemeta::hydra::http::ClientStream request{"https://www.example.com"};
  ///
  /// std::istringstream body{"foo bar baz"};
  /// request.on_body([&body](const std::size_t bytes) {
  ///   std::vector<std::uint8_t> result;
  ///   while (result.size() < bytes && request_body.peek() != EOF) {
  ///     result.push_back(static_cast<std::uint8_t>(request_body.get()));
  ///   }
  ///
  ///   return result;
  /// });
  ///
  /// request.send().wait();
  /// ```
  auto on_body(BodyCallback callback) noexcept -> void;

private:
  struct Internal;

  // No need to make this private, as the contents of `Internal`
  // are already hidden with the PIMPL idiom.
public:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::unique_ptr<Internal> internal;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::hydra::http

#endif
