#include <sourcemeta/jsontoolkit/uri.h>
#include <uriparser/Uri.h>

#include <cassert>   // assert
#include <cstdint>   // std::uint32_t
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::length_error, std::runtime_error
#include <string>    // std::stoul, std::string, std::tolower
#include <utility>   // std::move

static auto uri_normalize(UriUriA *uri) -> void {
  if (uriNormalizeSyntaxA(uri) != URI_SUCCESS) {
    throw sourcemeta::jsontoolkit::URIError{"Could not normalize URI"};
  }
}

static auto uri_to_string(const UriUriA *const uri) -> std::string {
  int size;
  if (uriToStringCharsRequiredA(uri, &size) != URI_SUCCESS) {
    throw sourcemeta::jsontoolkit::URIError{"Could not determine URI size"};
  }

  std::string result;
  // Because of this, `uriToStringA` could never error out with
  // `URI_ERROR_OUTPUT_TOO_LARGE`.
  result.resize(static_cast<std::string::size_type>(size));
  if (uriToStringA(result.data(), uri, size + 1, nullptr) != URI_SUCCESS) {
    throw sourcemeta::jsontoolkit::URIError{"Could not stringify URI"};
  }

  return result;
}

static auto uri_text_range(const UriTextRangeA *const range)
    -> std::optional<std::string_view> {
  if (range->afterLast == nullptr) {
    return std::nullopt;
  }

  return std::string_view{range->first,
                          static_cast<std::string_view::size_type>(
                              range->afterLast - range->first)};
}

static auto uri_parse(const std::string &data, UriUriA *uri) -> void {
  const char *error_position;
  switch (uriParseSingleUriA(uri, data.c_str(), &error_position)) {
    case URI_ERROR_SYNTAX:
      // TODO: Test the positions of this error
      throw sourcemeta::jsontoolkit::URIParseError{
          static_cast<std::uint64_t>(error_position - data.c_str() + 1)};
    case URI_ERROR_MALLOC:
      throw std::runtime_error("URI malloc error");
    case URI_ERROR_OUTPUT_TOO_LARGE:
      throw std::length_error("URI output too large");
    case URI_SUCCESS:
      break;
    default:
      throw sourcemeta::jsontoolkit::URIError{"Unknown URI error"};
  }

  uri_normalize(uri);
}

namespace sourcemeta::jsontoolkit {

struct URI::Internal {
  UriUriA uri;
};

URI::URI(std::string input) : data{std::move(input)}, internal{new Internal} {
  uri_parse(this->data, &this->internal->uri);
}

URI::~URI() { uriFreeUriMembersA(&this->internal->uri); }

URI::URI(const URI &other) : URI{other.recompose()} {}

URI::URI(URI &&other)
    : data{std::move(other.data)}, internal{std::move(other.internal)} {
  other.internal = nullptr;
}

auto URI::is_absolute() const noexcept -> bool {
  // An absolute URI always contains a scheme component,
  return this->internal->uri.scheme.first != nullptr;
}

auto URI::is_urn() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "urn";
}

auto URI::is_tag() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "tag";
}

auto URI::is_fragment_only() const -> bool {
  return !this->scheme().has_value() && !this->host().has_value() &&
         !this->port().has_value() && !this->path().has_value() &&
         this->fragment().has_value() && !this->query().has_value();
}

auto URI::scheme() const -> std::optional<std::string_view> {
  return uri_text_range(&this->internal->uri.scheme);
}

auto URI::host() const -> std::optional<std::string_view> {
  return uri_text_range(&this->internal->uri.hostText);
}

auto URI::port() const -> std::optional<std::uint32_t> {
  const auto port_text{uri_text_range(&this->internal->uri.portText)};
  if (!port_text.has_value()) {
    return std::nullopt;
  }

  return std::stoul(std::string{port_text.value()});
}

auto URI::path() const -> std::optional<std::string> {
  const UriPathSegmentA *segment{this->internal->uri.pathHead};
  if (!segment) {
    return std::nullopt;
  }

  // URNs and tags have a single path segment by definition
  if (this->is_urn() || this->is_tag()) {
    const auto part{uri_text_range(&segment->text)};
    assert(part.has_value());
    return std::string{part.value()};
  }

  std::ostringstream result;
  while (segment) {
    const auto part{uri_text_range(&segment->text)};
    assert(part.has_value());
    result << '/';
    result << part.value();
    segment = segment->next;
  }

  return result.str();
}

auto URI::fragment() const -> std::optional<std::string_view> {
  return uri_text_range(&this->internal->uri.fragment);
}

auto URI::query() const -> std::optional<std::string_view> {
  return uri_text_range(&this->internal->uri.query);
}

auto URI::recompose() const -> std::string {
  return uri_to_string(&this->internal->uri);
}

auto URI::recompose_without_fragment() const -> std::optional<std::string> {
  std::ostringstream result;

  // Scheme
  const auto result_scheme{this->scheme()};
  if (result_scheme.has_value()) {
    result << result_scheme.value();
    if (this->is_urn() || this->is_tag()) {
      result << ":";
    } else {
      result << "://";
    }
  }

  // Host
  const auto result_host{this->host()};
  if (result_host.has_value()) {
    result << result_host.value();
  }

  // Port
  const auto result_port{this->port()};
  if (result_port.has_value()) {
    result << ':' << result_port.value();
  }

  // Path
  const auto result_path{this->path()};
  if (result_path.has_value()) {
    result << result_path.value();
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result << '?' << result_query.value();
  }

  if (result.tellp() == 0) {
    return std::nullopt;
  }

  return result.str();
}

auto URI::canonicalize() -> URI & {
  std::ostringstream result;

  // Scheme
  const auto result_scheme{this->scheme()};
  if (result_scheme.has_value()) {
    for (const auto character : result_scheme.value()) {
      result << static_cast<char>(std::tolower(character));
    }

    if (this->is_urn() || this->is_tag()) {
      result << ":";
    } else {
      result << "://";
    }
  }

  // Host
  const auto result_host{this->host()};
  if (result_host.has_value()) {
    for (const auto character : result_host.value()) {
      result << static_cast<char>(std::tolower(character));
    }
  }

  // Port
  const auto result_port{this->port()};
  if (result_port.has_value()) {
    const bool is_default_http_port{result_scheme.has_value() &&
                                    result_scheme.value() == "http" &&
                                    result_port.value() == 80};
    const bool is_default_https_port{result_scheme.has_value() &&
                                     result_scheme.value() == "https" &&
                                     result_port.value() == 443};

    if (!is_default_http_port && !is_default_https_port) {
      result << ':' << result_port.value();
    }
  }

  // Path
  const auto result_path{this->path()};
  if (result_path.has_value()) {
    result << result_path.value();
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result << '?' << result_query.value();
  }

  // Fragment
  const auto result_fragment{this->fragment()};
  if (result_fragment.has_value() && !result_fragment.value().empty()) {
    result << '#' << result_fragment.value();
  }

  this->data = result.str();
  uriFreeUriMembersA(&this->internal->uri);
  uri_parse(this->data, &this->internal->uri);
  return *this;
}

auto URI::resolve_from(const URI &base) -> URI & {
  UriUriA absoluteDest;
  // Looks like this function allocates to the output variable
  // even on failure.
  // See https://uriparser.github.io/doc/api/latest/
  switch (uriAddBaseUriExA(&absoluteDest, &this->internal->uri,
                           &base.internal->uri, URI_RESOLVE_STRICTLY)) {
    case URI_SUCCESS:
      break;
    case URI_ERROR_ADDBASE_REL_BASE:
      uriFreeUriMembersA(&absoluteDest);
      assert(!base.is_absolute());
      throw URIError{"Base URI is not absolute"};
    default:
      uriFreeUriMembersA(&absoluteDest);
      throw URIError{"Could not resolve URI"};
  }

  try {
    uri_normalize(&absoluteDest);
    this->data = uri_to_string(&absoluteDest);
    uriFreeUriMembersA(&absoluteDest);
    uriFreeUriMembersA(&this->internal->uri);
    uri_parse(this->data, &this->internal->uri);
    return *this;
  } catch (...) {
    uriFreeUriMembersA(&absoluteDest);
    throw;
  }
}

auto URI::from_fragment(std::string_view fragment) -> URI {
  assert(fragment.empty() || fragment.front() != '#');
  std::ostringstream uri;
  uri << "#" << fragment;
  return {uri.str()};
}

auto URI::resolve_from_if_absolute(const URI &base) -> URI & {
  if (base.is_absolute()) {
    return this->resolve_from(base);
  } else {
    return *this;
  }
}

} // namespace sourcemeta::jsontoolkit
