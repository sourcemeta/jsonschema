#ifndef SOURCEMETA_CORE_BUILD_TYPES_H_
#define SOURCEMETA_CORE_BUILD_TYPES_H_

#include <functional> // std::function
#include <vector>     // std::vector

namespace sourcemeta::core {

/// @ingroup build
template <typename NodeType>
using BuildDynamicCallback = std::function<void(const NodeType &)>;

/// @ingroup build
/// Dependencies are encoded using an ordered sequence type
template <typename NodeType> using BuildDependencies = std::vector<NodeType>;

/// @ingroup build
template <typename Context, typename NodeType>
using BuildHandler =
    std::function<void(const NodeType &, const BuildDependencies<NodeType> &,
                       const BuildDynamicCallback<NodeType> &,
                       const Context &)>;

} // namespace sourcemeta::core

#endif // SOURCEMETA_CORE_OPTIONS_H_
