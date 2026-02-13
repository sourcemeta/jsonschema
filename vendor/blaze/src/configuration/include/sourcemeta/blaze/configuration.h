#ifndef SOURCEMETA_BLAZE_CONFIGURATION_H_
#define SOURCEMETA_BLAZE_CONFIGURATION_H_

/// @defgroup configuration Configuration
/// @brief A configuration manifest for JSON Schema projects
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/configuration.h>
/// ```

#ifndef SOURCEMETA_BLAZE_CONFIGURATION_EXPORT
#include <sourcemeta/blaze/configuration_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/configuration_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint8_t
#include <exception>     // std::exception_ptr
#include <filesystem>    // std::filesystem
#include <functional>    // std::function
#include <map>           // std::map
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup configuration
/// A configuration file format for JSON Schema projects
struct SOURCEMETA_BLAZE_CONFIGURATION_EXPORT Configuration {
  std::optional<sourcemeta::core::JSON::String> title;
  std::optional<sourcemeta::core::JSON::String> description;
  std::optional<sourcemeta::core::JSON::String> email;
  std::optional<sourcemeta::core::JSON::String> github;
  std::optional<sourcemeta::core::JSON::String> website;
  std::filesystem::path absolute_path;
  sourcemeta::core::JSON::String base;
  std::optional<sourcemeta::core::JSON::String> default_dialect;
  std::unordered_set<sourcemeta::core::JSON::String> extension{".json", ".yml",
                                                               ".yaml"};
  std::unordered_map<sourcemeta::core::JSON::String,
                     sourcemeta::core::JSON::String>
      resolve;
  // Ordered to guarantee deterministic iteration
  std::map<sourcemeta::core::JSON::String, std::filesystem::path> dependencies;
  sourcemeta::core::JSON extra = sourcemeta::core::JSON::make_object();

  /// A callback to read file contents from a path
  using ReadCallback =
      std::function<std::string(const std::filesystem::path &)>;

  /// A lock file that tracks fetched dependency hashes
  class SOURCEMETA_BLAZE_CONFIGURATION_EXPORT Lock {
  public:
    struct Entry {
      std::filesystem::path path;
      // TODO: Separately store the server ETag for exploiting HTTP caching?
      sourcemeta::core::JSON::String hash;
      enum class HashAlgorithm : std::uint8_t { SHA256 };
      HashAlgorithm hash_algorithm;
      enum class Status : std::uint8_t {
        Untracked,
        FileMissing,
        Mismatched,
        PathMismatch,
        UpToDate
      };
    };

    using const_iterator =
        std::map<sourcemeta::core::JSON::String, Entry>::const_iterator;

    auto
    emplace(const sourcemeta::core::JSON::String &uri,
            const std::filesystem::path &path,
            const sourcemeta::core::JSON::String &hash,
            Entry::HashAlgorithm hash_algorithm = Entry::HashAlgorithm::SHA256)
        -> void;
    auto erase(const sourcemeta::core::JSON::String &uri) -> void;

    [[nodiscard]] auto size() const noexcept -> std::size_t;
    [[nodiscard]] auto at(const sourcemeta::core::JSON::String &uri) const
        -> std::optional<std::reference_wrapper<const Entry>>;
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    [[nodiscard]] auto end() const noexcept -> const_iterator;

    [[nodiscard]]
    auto check(const sourcemeta::core::JSON::String &uri,
               const std::filesystem::path &expected_path,
               const ReadCallback &reader) const -> Entry::Status;
    [[nodiscard]]
    static auto from_json(const sourcemeta::core::JSON &value) -> Lock;
    [[nodiscard]]
    auto to_json() const -> sourcemeta::core::JSON;

  private:
    // Ordered to guarantee deterministic iteration
    std::map<sourcemeta::core::JSON::String, Entry> entries_;
  };

  /// An event emitted during dependency fetching
  struct FetchEvent {
    enum class Type : std::uint8_t {
      FetchStart,
      FetchEnd,
      BundleStart,
      BundleEnd,
      WriteStart,
      WriteEnd,
      VerifyStart,
      VerifyEnd,
      UpToDate,
      FileMissing,
      Orphaned,
      Mismatched,
      PathMismatch,
      Untracked,
      Error
    };

    Type type;
    std::string uri;
    std::filesystem::path path;
    std::size_t index;
    std::size_t total;
    std::string details;
    std::exception_ptr exception;

    using Callback = std::function<bool(const FetchEvent &)>;
  };

  /// A callback to fetch a JSON Schema from a URI
  using FetchCallback =
      std::function<sourcemeta::core::JSON(std::string_view uri)>;

  /// A callback to write a JSON Schema to a path
  using WriteCallback = std::function<void(const std::filesystem::path &,
                                           const sourcemeta::core::JSON &)>;

  /// The mode for fetching dependencies with a mutable lock
  enum class FetchMode : std::uint8_t {
    /// Fetch only what's missing or untracked
    Missing,
    /// Re-fetch everything regardless of current state
    All
  };

  /// Fetch dependencies, modifying the lock in-place
  auto fetch(Lock &lock, const FetchCallback &fetcher,
             const sourcemeta::core::SchemaResolver &resolver,
             const ReadCallback &reader, const WriteCallback &writer,
             const FetchEvent::Callback &on_event, FetchMode mode,
             // TODO: Make this work for real
             std::size_t concurrency = 1) const -> void;

  /// Fetch dependencies without modifying the lock file (frozen mode)
  auto fetch(const Lock &lock, const FetchCallback &fetcher,
             const sourcemeta::core::SchemaResolver &resolver,
             const ReadCallback &reader, const WriteCallback &writer,
             const FetchEvent::Callback &on_event, bool dry_run = false,
             // TODO: Make this work for real
             std::size_t concurrency = 1) const -> void;

  /// Add a dependency to the configuration
  auto add_dependency(const sourcemeta::core::URI &uri,
                      const std::filesystem::path &path) -> void;

  /// Check if the given path represents a schema described by this
  /// configuration
  [[nodiscard]]
  auto applies_to(const std::filesystem::path &path) const -> bool;

  /// Parse a configuration file from its contents
  [[nodiscard]]
  static auto from_json(const sourcemeta::core::JSON &value,
                        const std::filesystem::path &base_path)
      -> Configuration;

  /// Serialize a configuration to JSON
  [[nodiscard]]
  auto to_json() const -> sourcemeta::core::JSON;

  /// Read and parse a configuration file
  [[nodiscard]]
  static auto read_json(const std::filesystem::path &path,
                        const ReadCallback &reader) -> Configuration;

  /// A nearest ancestor configuration lookup
  [[nodiscard]]
  static auto find(const std::filesystem::path &path)
      -> std::optional<std::filesystem::path>;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
