#ifndef SOURCEMETA_CORE_BUILD_ADAPTER_FILESYSTEM_H_
#define SOURCEMETA_CORE_BUILD_ADAPTER_FILESYSTEM_H_

#ifndef SOURCEMETA_CORE_BUILD_EXPORT
#include <sourcemeta/core/build_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/build_types.h>
// NOLINTEND(misc-include-cleaner)

#include <filesystem>    // std::filesystem
#include <mutex>         // std::mutex, std::lock_guard
#include <optional>      // std::optional
#include <string>        // std::string
#include <unordered_map> // std::unordered_map

namespace sourcemeta::core {

/// @ingroup build
class SOURCEMETA_CORE_BUILD_EXPORT BuildAdapterFilesystem {
public:
  using node_type = std::filesystem::path;
  using mark_type = std::filesystem::file_time_type;

  BuildAdapterFilesystem() = default;
  BuildAdapterFilesystem(std::string dependency_extension);

  [[nodiscard]] auto dependencies_path(const node_type &path) const
      -> node_type;
  [[nodiscard]] auto read_dependencies(const node_type &path) const
      -> std::optional<BuildDependencies<node_type>>;
  auto write_dependencies(const node_type &path,
                          const BuildDependencies<node_type> &dependencies)
      -> void;
  auto refresh(const node_type &path) -> void;

  [[nodiscard]] auto mark(const node_type &path) const
      -> std::optional<mark_type>;

  [[nodiscard]] auto is_newer_than(const mark_type left,
                                   const mark_type right) const -> bool;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  std::string extension{".deps"};
  std::unordered_map<node_type, mark_type> marks;
  std::mutex mutex;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::core

#endif // SOURCEMETA_CORE_OPTIONS_H_
