#ifndef SOURCEMETA_CORE_URI_H_
#define SOURCEMETA_CORE_URI_H_

#ifndef SOURCEMETA_CORE_URI_EXPORT
#include <sourcemeta/core/uri_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/uri_error.h>
#include <sourcemeta/core/uri_escape.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint>     // std::uint32_t
#include <filesystem>  // std::filesystem
#include <istream>     // std::istream
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

/// @defgroup uri URI
/// @brief A RFC 3986 URI implementation based on `uriparser`.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// ```

namespace sourcemeta::core {

/// @ingroup uri
class SOURCEMETA_CORE_URI_EXPORT URI {
public:
  // TODO: Add a constructor that takes a C++ input stream

  /// This constructor creates a URI from a string type. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  ///
  /// const sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// ```
  URI(std::string input);

  /// This constructor creates a URI from a C++ input stream. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <sstream>
  ///
  /// std::istringstream input{"https://www.sourcemeta.com"};
  /// const sourcemeta::core::URI uri{input};
  /// ```
  URI(std::istream &input);

  /// Destructor
  ~URI();

  /// Copy constructor
  URI(const URI &other);

  /// Move constructor
  URI(URI &&other) noexcept;

  /// Check if the URI is absolute. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// assert(uri.is_absolute());
  /// ```
  [[nodiscard]] auto is_absolute() const noexcept -> bool;

  /// Check if the URI is a URN. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"urn:example:schema"};
  /// assert(uri.is_urn());
  /// ```
  auto is_urn() const -> bool;

  /// Check if the URI is a tag as described by RFC 4151. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"tag:yaml.org,2002:int"};
  /// assert(uri.is_tag());
  /// ```
  auto is_tag() const -> bool;

  /// Check if the URI has the `mailto` scheme. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"mailto:joe@example.com"};
  /// assert(uri.is_mailto());
  /// ```
  auto is_mailto() const -> bool;

  /// Check if the URI only consists of a fragment. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"#foo"};
  /// assert(uri.is_fragment_only());
  /// ```
  auto is_fragment_only() const -> bool;

  /// Check if the URI is relative. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"./foo"};
  /// assert(uri.is_relative());
  /// ```
  auto is_relative() const -> bool;

  /// Check if the host is an ipv6 address. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"http://[::1]"};
  /// assert(uri.is_ipv6());
  /// ```
  auto is_ipv6() const -> bool;

  /// Check if the URI corresponds to the empty URI. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{""};
  /// assert(uri.empty());
  /// ```
  auto empty() const -> bool;

  /// Get the scheme part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// assert(uri.scheme().has_value());
  /// assert(uri.scheme().value() == "https");
  /// ```
  [[nodiscard]] auto scheme() const -> std::optional<std::string_view>;

  /// Get the host part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// assert(uri.host().has_value());
  /// assert(uri.host().value() == "sourcemeta.com");
  /// ```
  [[nodiscard]] auto host() const -> std::optional<std::string_view>;

  /// Get the port part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"http://localhost:8000"};
  /// assert(uri.port().has_value());
  /// assert(uri.port().value() == 8000);
  /// ```
  [[nodiscard]] auto port() const -> std::optional<std::uint32_t>;

  /// Get the path part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI
  /// uri{"https://www.sourcemeta.com/foo/bar"};
  /// assert(uri.path().has_value());
  /// assert(uri.path().value() == "/foo/bar");
  /// ```
  [[nodiscard]] auto path() const -> std::optional<std::string>;

  /// Set the path part of the URI. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// const std::string path{"/foo/bar"};
  /// uri.path(path);
  /// assert(uri.path().has_value());
  /// assert(uri.path().value() == "/foo/bar");
  /// ```
  auto path(const std::string &path) -> URI &;

  /// Set the path part of the URI with move semantics. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// std::string path{"/foo/bar"};
  /// uri.path(std::move(path));
  /// assert(uri.path().has_value());
  /// assert(uri.path().value() == "/foo/bar");
  auto path(std::string &&path) -> URI &;

  /// Append a path to the existing URI path or set a path if such component
  /// does not exist in the URI. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com/foo"};
  /// uri.append_path("bar/baz");
  /// assert(uri.recompose() == "https://www.sourcemeta.com/foo/bar/baz");
  auto append_path(const std::string &path) -> URI &;

  /// If the URI has a path, this method sets or replace the extension in the
  /// path. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com/foo"};
  /// uri.extension("json");
  /// assert(uri.recompose() == "https://www.sourcemeta.com/foo.json");
  auto extension(std::string &&extension) -> URI &;

  /// Get the fragment part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"https://www.sourcemeta.com/#foo"};
  /// assert(uri.fragment().has_value());
  /// assert(uri.fragment().value() == "foo");
  /// ```
  [[nodiscard]] auto fragment() const -> std::optional<std::string_view>;

  /// Set the fragment part of the URI. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// const std::string fragment{"foo"};
  /// uri.fragment(fragment);
  /// assert(uri.fragment().has_value());
  /// assert(uri.fragment().value() == "foo");
  /// ```
  auto fragment(const std::string &fragment) -> URI &;

  /// Set the fragment part of the URI with move semantics. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://www.sourcemeta.com"};
  /// std::string fragment{"foo"};
  /// uri.fragment(std::move(fragment));
  /// assert(uri.fragment().has_value());
  /// assert(uri.fragment().value() == "foo");
  auto fragment(std::string &&fragment) -> URI &;

  /// Get the non-dissected query part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI
  ///   uri{"https://www.sourcemeta.com/?foo=bar"};
  /// assert(uri.query().has_value());
  /// assert(uri.query().value() == "foo=bar");
  /// ```
  [[nodiscard]] auto query() const -> std::optional<std::string_view>;

  /// Recompose a URI as established by RFC 3986. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI
  ///   uri{"https://www.sourcemeta.com/foo/../bar"};
  /// assert(uri.recompose() == "https://sourcemeta.com/bar");
  /// ```
  [[nodiscard]] auto recompose() const -> std::string;

  /// Recompose a URI as established by RFC 3986, but without including the
  /// fragment component. The result is an optional to handle the case where the
  /// input URI only consists of a fragment. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI
  ///   uri{"https://www.sourcemeta.com/foo#bar"};
  /// assert(uri.recompose_without_fragment().has_value()");
  /// assert(uri.recompose_without_fragment().value() ==
  /// "https://sourcemeta.com/foo");
  /// ```
  [[nodiscard]] auto recompose_without_fragment() const
      -> std::optional<std::string>;

  /// Canonicalize a URI. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"hTtP://exAmpLe.com:80/TEST"};
  /// uri.canonicalize():
  /// assert(uri.recompose() == "http://example.com/TEST");
  /// ```
  auto canonicalize() -> URI &;

  /// Resolve a relative URI against a base URI as established by RFC 3986. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI base{"https://www.sourcemeta.com"};
  /// sourcemeta::core::URI result{"foo"};
  /// result.resolve_from(base);
  /// assert(result.recompose() == "https://sourcemeta.com/foo");
  /// ```
  auto resolve_from(const URI &base) -> URI &;

  // TODO: Do we really need this `try_resolve_from` method? There shouldn't
  // be any reason why resolution cannot happen. This is probably just an
  // artifact of `uriparser` not supporting relative resolution

  /// Resolve a relative URI against a base URI as established by RFC
  /// 3986. If the resolution cannot happen, nothing happens. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI base{"bar"};
  /// sourcemeta::core::URI result{"foo"};
  /// result.try_resolve_from(base);
  /// assert(result.recompose() == "foo");
  /// ```
  auto try_resolve_from(const URI &base) -> URI &;

  /// Attempt to resolve a URI relative to another URI. If the latter URI is not
  /// a base for the former, leave the URI intact. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI base{"https://www.sourcemeta.com"};
  /// sourcemeta::core::URI result{"https://www.sourcemeta.com/foo"};
  /// result.relative_to(base);
  /// assert(result.recompose() == "foo");
  /// ```
  auto relative_to(const URI &base) -> URI &;

  /// Attempt to change the base of a URI . If the URI is not
  /// relative to the former, leave the URI intact. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::URI uri{"https://example.com/foo/bar/baz"};
  /// const sourcemeta::core::URI base{"https://example.com/foo"};
  /// const sourcemeta::core::URI new_base{"/qux"};
  /// uri.rebase(base, new_base);
  /// assert(uri.recompose() == "/qux/bar/baz");
  /// ```
  auto rebase(const URI &base, const URI &new_base) -> URI &;

  /// Get the user information part of the URI, if any. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{"https://user:@host"};
  /// assert(uri.userinfo().has_value());
  /// assert(uri.userinfo().value() == "user:);
  /// ```
  ///
  /// As mentioned in RFC 3986, the format "user:password" is deprecated.
  /// Applications should not render as clear text any data after the first
  /// colon. See https://tools.ietf.org/html/rfc3986#section-3.2.1
  [[nodiscard]] auto userinfo() const -> std::optional<std::string_view>;

  /// To support equality of URIs
  auto operator==(const URI &other) const noexcept -> bool;

  /// To support ordering of URIs
  auto operator<(const URI &other) const noexcept -> bool;

  /// Create a URI from a fragment. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::URI uri{
  ///   sourcemeta::core::URI::from_fragment("foo")};
  /// assert(uri.recompose() == "#foo");
  /// ```
  static auto from_fragment(std::string_view fragment) -> URI;

  /// Create a URI from a file system path. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  /// #include <filesystem>
  ///
  /// const std::filesystem::path path{"/foo/bar"};
  /// const sourcemeta::core::URI uri{sourcemeta::core::URI::from_path(path)};
  /// assert(uri.recompose() == "file:///foo/bar");
  /// ```
  static auto from_path(const std::filesystem::path &path) -> URI;

  /// A convenient method to canonicalize and recompose a URI from a string. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/uri.h>
  /// #include <cassert>
  ///
  /// const auto result{
  ///   sourcemeta::core::URI::canonicalize("hTtP://exAmpLe.com:80/TEST")};
  /// assert(result == "http://example.com/TEST");
  /// ```
  static auto canonicalize(const std::string &input) -> std::string;

private:
  bool parsed = false;
  auto parse() -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string data{};

  std::optional<std::string> path_{};
  std::optional<std::string> userinfo_{};
  std::optional<std::string> host_{};
  std::optional<std::uint32_t> port_{};
  std::optional<std::string> scheme_{};
  std::optional<std::string> fragment_{};
  std::optional<std::string> query_{};
  bool is_ipv6_ = false;

  // Use PIMPL idiom to hide `uriparser`
  struct Internal;
  std::unique_ptr<Internal> internal{};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
