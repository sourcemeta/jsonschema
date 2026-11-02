#ifndef SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_
#define SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <sourcemeta/core/io.h>

#include <cstddef> // std::size_t
#include <cstdint> // std::uint16_t, std::uint32_t, std::uint8_t, std::int64_t
#include <filesystem>    // std::filesystem::path
#include <functional>    // std::function
#include <memory>        // std::unique_ptr
#include <span>          // std::span
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility>       // std::pair
#include <variant>       // std::variant
#include <vector>        // std::vector

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup uritemplate
/// A URI Template path router. Keep in mind that the URI Template specification
/// DOES NOT define matching, only expansion. So this is an opinionated
/// non-standard adaptation of URI Template for path routing purposes. The
/// supported operators are:
///
/// - `{var}` for a single path segment (RFC 6570 Level 1 simple expansion)
/// - `{+var}` for greedy capture to the end of the path, requiring at least
///   one trailing segment (RFC 6570 Level 2 reserved expansion)
/// - `{/var*}` for optional greedy capture to the end of the path, where
///   zero trailing segments are also allowed (RFC 6570 Level 3 path-segment
///   operator with Level 4 explode modifier). Must be the last component of
///   the template. The captured value is empty when no trailing segments
///   are present
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

  /// The value of a route argument
  using ArgumentValue = std::variant<std::string_view, std::int64_t, bool>;

  /// A named route argument
  using Argument = std::pair<std::string_view, ArgumentValue>;

  /// The argument callback (name, value)
  using ArgumentCallback =
      std::function<void(std::string_view, const ArgumentValue &)>;

  /// The type of a node in the router trie
  enum class NodeType : std::uint8_t {
    /// The root of the trie
    Root = 0,
    /// A fixed literal path segment
    Literal = 1,
    /// A single path segment capture
    Variable = 2,
    /// A greedy capture requiring at least one trailing segment
    Expansion = 3,
    /// A greedy capture allowing zero trailing segments
    OptionalExpansion = 4
  };

  /// A node in the router trie
  struct Node {
    /// The handler identifier of the route ending at this node
    Identifier identifier{0};
    /// The context identifier associated with this node
    Identifier context{0};
    /// The kind of component this node represents
    NodeType type{NodeType::Root};
    /// The literal text or variable name of this node
    std::string_view value;

    // This children distinction enforces that there can only be one non-literal
    // child at the type level. Also allows us to more efficiently search on
    // literals
    /// The literal children of this node
    std::vector<std::unique_ptr<Node>> literals;
    /// The single non-literal child of this node
    std::unique_ptr<Node> variable;
  };

  /// Construct an empty router
  URITemplateRouter() = default;

  /// Construct a router with a base path prefix. During matching, the base
  /// path is stripped from incoming request paths before matching. An optional
  /// base URL can be associated with the router as opaque metadata, never used
  /// for matching
  explicit URITemplateRouter(std::string_view base_path,
                             std::string_view base_url = {});

  // To avoid mistakes
  URITemplateRouter(const URITemplateRouter &) = delete;
  URITemplateRouter(URITemplateRouter &&) = delete;
  auto operator=(const URITemplateRouter &) -> URITemplateRouter & = delete;
  auto operator=(URITemplateRouter &&) -> URITemplateRouter & = delete;

  /// Add a route to the router. Make sure the string lifetime survives the
  /// router. The operation identifier must match the regular expression
  /// `^[a-zA-Z][a-zA-Z0-9_-]{0,63}$` and must be unique across all routes
  /// registered on this router
  auto add(const std::string_view uri_template,
           const std::string_view operation_id, const Identifier identifier,
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

  /// Determine whether a path lies within the space described by the explicitly
  /// registered routes, as a whole-segment prefix of one or more routes, as an
  /// exact route, or as a path captured by a route expansion. The fallback
  /// registered through the catch-all is never considered. Like matching, the
  /// path is not normalized before evaluation. When a base path is given, it is
  /// evaluated as if prepended to the path, avoiding a concatenation at the
  /// call site
  [[nodiscard]] auto
  describes(const std::string_view path,
            const std::string_view base_path = {}) const noexcept -> bool;

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

  /// Access the base URL associated with the router
  [[nodiscard]] auto base_url() const noexcept -> std::string_view;

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

  /// Resolve an operation identifier to its registered route. Returns
  /// `(identifier, context)` on hit and `(0, 0)` if no route is registered
  /// under the given operation identifier
  [[nodiscard]] auto operation(const std::string_view operation_id) const
      -> std::pair<Identifier, Identifier>;

  /// Get the operation identifier associated with a registered route
  /// identifier
  [[nodiscard]] auto operation_id(const Identifier identifier) const
      -> std::string_view;

private:
  Node root_;
  Node otherwise_;
  std::string base_path_;
  std::string base_url_;
  std::vector<std::pair<Identifier, std::vector<Argument>>> arguments_;
  std::vector<std::tuple<Identifier, Identifier, std::string_view>> entries_;
  std::unordered_map<std::string_view, std::pair<Identifier, Identifier>>
      operations_;
};

/// @ingroup uritemplate
/// A read-only view of a serialized URI Template router
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterView {
public:
  /// Save a router to a binary file
  static auto save(const URITemplateRouter &router,
                   const std::filesystem::path &path) -> void;

  /// Construct a view by loading a serialized router from a file
  URITemplateRouterView(const std::filesystem::path &path);

  /// Construct a view over an externally-owned buffer. The buffer must
  /// outlive the view
  URITemplateRouterView(const std::uint8_t *data, std::size_t size);

  ~URITemplateRouterView();

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

  /// Determine whether a path lies within the space described by the explicitly
  /// registered routes, as a whole-segment prefix of one or more routes, as an
  /// exact route, or as a path captured by a route expansion. The fallback
  /// registered through the catch-all is never considered. Like matching, the
  /// path is not normalized before evaluation. When a base path is given, it is
  /// evaluated as if prepended to the path, avoiding a concatenation at the
  /// call site
  [[nodiscard]] auto
  describes(const std::string_view path,
            const std::string_view base_path = {}) const noexcept -> bool;

  /// Access the stored arguments for a given route identifier
  auto arguments(const URITemplateRouter::Identifier identifier,
                 const URITemplateRouter::ArgumentCallback &callback) const
      -> void;

  /// Access the base path prefix
  [[nodiscard]] auto base_path() const noexcept -> std::string_view;

  /// Access the base URL associated with the router
  [[nodiscard]] auto base_url() const noexcept -> std::string_view;

  /// Get the number of registered routes
  [[nodiscard]] auto size() const noexcept -> std::size_t;

  /// Resolve an operation identifier to its registered route. Returns
  /// `(identifier, context)` on hit and `(0, 0)` if no route is registered
  /// under the given operation identifier
  [[nodiscard]] auto operation(const std::string_view operation_id) const
      -> std::pair<URITemplateRouter::Identifier,
                   URITemplateRouter::Identifier>;

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

  /// Get the operation identifier associated with a registered route
  /// identifier
  [[nodiscard]] auto
  operation_id(const URITemplateRouter::Identifier identifier) const
      -> std::string_view;

private:
  const std::uint8_t *data_{nullptr};
  std::size_t size_{0};
  std::unique_ptr<FileView> owner_;
  // Holds an eight-byte-aligned copy of an unaligned external buffer, so the
  // over-aligned serialized nodes are always read from aligned storage
  std::vector<std::uint64_t> owned_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
