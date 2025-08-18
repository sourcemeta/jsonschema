#ifndef SOURCEMETA_CORE_BUILD_H_
#define SOURCEMETA_CORE_BUILD_H_

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/build_adapter_filesystem.h>
#include <sourcemeta/core/build_types.h>
// NOLINTEND(misc-include-cleaner)

#include <algorithm> // std::ranges::none_of
#include <cassert>   // assert

/// @defgroup build Build
/// @brief A simple and minimalistic Make-style build system
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/build.h>
/// ```

namespace sourcemeta::core {

/// @ingroup build
///
/// Run a handler given a destination node, a set of dependency nodes, and a
/// context value. The semantics of how nodes are retrieved, stored, and checked
/// for freshness depends on the adapter struct passed into the builder. If a
/// node is considered fresh given its static or dynamic dependencies, the
/// actual handler is never executed.
///
/// Here is an example using the `BuildAdapterFilesystem` pre-built
/// opinionated adapter:
///
/// ```c++
/// #include <sourcemeta/core/build.h>
///
/// auto HANDLER_COPY(
///     const std::filesystem::path &destination,
///     const sourcemeta::core::BuildDependencies<std::filesystem::path>
///       &dependencies,
///     // You may invoke this callback to register a dynamic dependency
///     const sourcemeta::core::BuildDynamicCallback<std::filesystem::path> &,
///     const std::filesystem::copy_options &options) -> void {
///   std::filesystem::create_directories(destination.parent_path());
///   std::filesystem::copy_file(dependencies.front(), destination, options);
/// }
///
/// sourcemeta::core::BuildAdapterFilesystem adapter;
///
/// // The file will be generated
/// sourcemeta::core::build<std::filesystem::copy_options>(
///   adapter, HANDLER_COPY,
///   "/tmp/destination.txt", {"/tmp/dependency.txt"},
///   std::filesystem::copy_options::overwrite_existing);
///
/// // The second time will result in a cache hit
/// sourcemeta::core::build<std::filesystem::copy_options>(
///   adapter, HANDLER_COPY,
///   "/tmp/destination.txt", {"/tmp/dependency.txt"},
///   std::filesystem::copy_options::overwrite_existing);
/// ```
template <typename Context, typename Adapter>
auto build(Adapter &adapter,
           const BuildHandler<Context, typename Adapter::node_type> &handler,
           const typename Adapter::node_type &destination,
           const BuildDependencies<typename Adapter::node_type> &dependencies,
           const Context &context) -> bool {
  const auto destination_mark{adapter.mark(destination)};
  const auto destination_dependencies{adapter.read_dependencies(destination)};
  if (destination_mark.has_value() && destination_dependencies.has_value() &&
      std::ranges::none_of(
          destination_dependencies.value(),
          [&adapter, &destination_mark](const auto &dependency) {
            const auto dependency_mark = adapter.mark(dependency);
            return !dependency_mark.has_value() ||
                   adapter.is_newer_than(dependency_mark.value(),
                                         destination_mark.value());
          })) {
    return false;
  }

  BuildDependencies<typename Adapter::node_type> deps;
  for (const auto &dependency : dependencies) {
    assert(adapter.mark(dependency).has_value());
    deps.emplace_back(dependency);
  }

  handler(
      destination, dependencies,
      // This callback is used for build handlers to dynamically populate
      // dependencies. This is very powerful for defining handlers where
      // the actual list of dependencies is only known while or after
      // processing the handler
      [&](const auto &new_dependency) {
        assert(adapter.mark(new_dependency).has_value());
        deps.emplace_back(new_dependency);
      },
      context);

  adapter.write_dependencies(destination, deps);
  return true;
}

} // namespace sourcemeta::core

#endif // SOURCEMETA_CORE_OPTIONS_H_
