#ifndef SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_
#define SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <cstddef> // std::size_t
#include <cstdint> // std::uint16_t, std::uint32_t, std::uint8_t, std::int64_t
#include <filesystem>  // std::filesystem::path
#include <functional>  // std::function
#include <memory>      // std::unique_ptr
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::tuple
#include <utility>     // std::pair
#include <variant>     // std::variant
#include <vector>      // std::vector

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup uritemplate
/// A URI Template path router. Keep in mind that the URI Template specification
/// DOES NOT define expansion. So this is an opinionated non-standard adaptation
/// of URI Template for path routing purposes
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouter {
  friend class URITemplateRouterView;

public:
  /// A handler identifier 0 means "no handler"
  using Identifier = std::uint16_t;

  /// The variable index type
  using Index = std::uint8_t;

  /// The match callback (index, name, value)
  using Callback =
      std::function<void(Index, std::string_view, std::string_view)>;

  using ArgumentValue = std::variant<std::string_view, std::int64_t, bool>;
  using Argument = std::pair<std::string_view, ArgumentValue>;
  using ArgumentCallback =
      std::function<void(std::string_view, const ArgumentValue &)>;

  /// The type of a node in the router trie
  enum class NodeType : std::uint8_t {
    Root = 0,
    Literal = 1,
    Variable = 2,
    Expansion = 3
  };

  /// A node in the router trie
  struct Node {
    Identifier identifier{0};
    Identifier context{0};
    NodeType type{NodeType::Root};
    std::string_view value;

    // This children distinction enforces that there can only be one non-literal
    // child at the type level. Also allows us to more efficiently search on
    // literals
    std::vector<std::unique_ptr<Node>> literals;
    std::unique_ptr<Node> variable;
  };

  /// Construct an empty router
  URITemplateRouter() = default;

  /// Construct a router with a base path prefix. During matching, the base
  /// path is stripped from incoming request paths before matching
  explicit URITemplateRouter(std::string_view base_path);

  // To avoid mistakes
  URITemplateRouter(const URITemplateRouter &) = delete;
  URITemplateRouter(URITemplateRouter &&) = delete;
  auto operator=(const URITemplateRouter &) -> URITemplateRouter & = delete;
  auto operator=(URITemplateRouter &&) -> URITemplateRouter & = delete;

  /// Add a route to the router. Make sure the string lifetime survives the
  /// router
  auto add(const std::string_view uri_template, const Identifier identifier,
           const Identifier context = 0,
           const std::span<const Argument> arguments = {}) -> void;

  /// Register a fallback context and arguments to be returned when matching
  /// a path that does not correspond to any registered route
  auto otherwise(const Identifier context,
                 const std::span<const Argument> arguments = {}) -> void;

  /// Match a path against the router. Note the callback might fire for
  /// initial matches even though the entire match might still fail
  [[nodiscard]] auto match(const std::string_view path,
                           const Callback &callback) const
      -> std::pair<Identifier, Identifier>;

  /// Access the root node of the trie
  [[nodiscard]] auto root() const noexcept -> const Node &;

  /// Access the stored arguments for a given route identifier
  auto arguments(const Identifier identifier,
                 const ArgumentCallback &callback) const -> void;

  /// Access all stored route arguments
  [[nodiscard]] auto arguments() const noexcept
      -> const std::vector<std::pair<Identifier, std::vector<Argument>>> &;

  /// Access the base path prefix
  [[nodiscard]] auto base_path() const noexcept -> std::string_view;

  /// Get the number of registered routes
  [[nodiscard]] auto size() const noexcept -> std::size_t;

  /// Get the identifier of the route at the given positional index
  [[nodiscard]] auto at(const std::size_t index) const -> Identifier;

  /// Get the context identifier associated with a registered route
  /// identifier
  [[nodiscard]] auto context(const Identifier identifier) const -> Identifier;

  /// Reconstruct and return the URI Template path string originally registered
  /// for the given identifier
  [[nodiscard]] auto path(const Identifier identifier) const -> std::string;

private:
  Node root_;
  Node otherwise_;
  std::string base_path_;
  std::vector<std::pair<Identifier, std::vector<Argument>>> arguments_;
  std::vector<std::tuple<Identifier, Identifier, std::string_view>> entries_;
};

/// @ingroup uritemplate
/// A read-only view of a serialized URI Template router
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterView {
public:
  /// Save a router to a binary file
  static auto save(const URITemplateRouter &router,
                   const std::filesystem::path &path) -> void;

  URITemplateRouterView(const std::filesystem::path &path);
  URITemplateRouterView(const std::uint8_t *data, std::size_t size);

  // To avoid mistakes
  URITemplateRouterView(const URITemplateRouterView &) = delete;
  URITemplateRouterView(URITemplateRouterView &&) = delete;
  auto operator=(const URITemplateRouterView &)
      -> URITemplateRouterView & = delete;
  auto operator=(URITemplateRouterView &&) -> URITemplateRouterView & = delete;

  /// Match a path against the router. Note the callback might fire for
  /// initial matches even though the entire match might still fail
  [[nodiscard]] auto match(const std::string_view path,
                           const URITemplateRouter::Callback &callback) const
      -> std::pair<URITemplateRouter::Identifier,
                   URITemplateRouter::Identifier>;

  /// Access the stored arguments for a given route identifier
  auto arguments(const URITemplateRouter::Identifier identifier,
                 const URITemplateRouter::ArgumentCallback &callback) const
      -> void;

  /// Access the base path prefix
  [[nodiscard]] auto base_path() const noexcept -> std::string_view;

  /// Get the number of registered routes
  [[nodiscard]] auto size() const noexcept -> std::size_t;

  /// Get the identifier of the route at the given positional index
  [[nodiscard]] auto at(const std::size_t index) const
      -> URITemplateRouter::Identifier;

  /// Get the context identifier associated with a registered route
  /// identifier
  [[nodiscard]] auto
  context(const URITemplateRouter::Identifier identifier) const
      -> URITemplateRouter::Identifier;

  /// Reconstruct and return the URI Template path string originally registered
  /// for the given identifier
  [[nodiscard]] auto path(const URITemplateRouter::Identifier identifier) const
      -> std::string;

private:
  std::vector<std::uint8_t> data_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
