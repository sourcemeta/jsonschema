#ifndef SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_
#define SOURCEMETA_CORE_URITEMPLATE_ROUTER_H_

#ifndef SOURCEMETA_CORE_URITEMPLATE_EXPORT
#include <sourcemeta/core/uritemplate_export.h>
#endif

#include <sourcemeta/core/io.h>

#include <cstdint>     // std::uint16_t, std::uint32_t, std::uint8_t
#include <filesystem>  // std::filesystem::path
#include <functional>  // std::function
#include <memory>      // std::unique_ptr
#include <string_view> // std::string_view
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
public:
  /// A handler identifier 0 means "no handler"
  using Identifier = std::uint16_t;

  /// The variable index type
  using Index = std::uint8_t;

  /// The match callback (index, name, value)
  using Callback =
      std::function<void(Index, std::string_view, std::string_view)>;

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

  // To avoid mistakes
  URITemplateRouter(const URITemplateRouter &) = delete;
  URITemplateRouter(URITemplateRouter &&) = delete;
  auto operator=(const URITemplateRouter &) -> URITemplateRouter & = delete;
  auto operator=(URITemplateRouter &&) -> URITemplateRouter & = delete;

  /// Add a route to the router. Make sure the string lifetime survives the
  /// router
  auto add(const std::string_view uri_template, const Identifier identifier)
      -> void;

  /// Match a path against the router. Note the callback might fire for
  /// initial matches even though the entire match might still fail
  [[nodiscard]] auto match(const std::string_view path,
                           const Callback &callback) const -> Identifier;

  /// Access the root node of the trie
  [[nodiscard]] auto root() const noexcept -> const Node &;

private:
  Node root_;
};

/// @ingroup uritemplate
/// A read-only memory-mapped view of a serialized URI Template router
class SOURCEMETA_CORE_URITEMPLATE_EXPORT URITemplateRouterView {
public:
  /// A serialized node in the binary format
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  struct alignas(8) Node {
    std::uint32_t string_offset;
    std::uint32_t string_length;
    std::uint32_t first_literal_child;
    std::uint32_t literal_child_count;
    std::uint32_t variable_child;
    URITemplateRouter::NodeType type;
    std::uint8_t padding;
    URITemplateRouter::Identifier identifier;
  };
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

  /// Save a router to a binary file
  static auto save(const URITemplateRouter &router,
                   const std::filesystem::path &path) -> void;

  URITemplateRouterView(const std::filesystem::path &path);

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
      -> URITemplateRouter::Identifier;

private:
  FileView file_view_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
