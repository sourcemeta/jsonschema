#ifndef SOURCEMETA_CORE_HTML_ENCODER_H_
#define SOURCEMETA_CORE_HTML_ENCODER_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/html_escape.h>

#include <iostream> // std::ostream
#include <string>   // std::string
#include <utility>  // std::pair
#include <variant>  // std::variant, std::holds_alternative, std::get
#include <vector>   // std::vector

namespace sourcemeta::core {

/// @ingroup html
using HTMLAttributes = std::vector<std::pair<std::string, std::string>>;

#ifndef DOXYGEN
// Forward declaration
class HTML;
#endif

/// @ingroup html
/// Raw HTML content wrapper for unescaped content
struct SOURCEMETA_CORE_HTML_EXPORT HTMLRaw {
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string content;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  explicit HTMLRaw(std::string html_content)
      : content{std::move(html_content)} {}
};

/// @ingroup html
/// A node can be either a string (text node), raw HTML content, or another HTML
/// element
using HTMLNode = std::variant<std::string, HTMLRaw, HTML>;

/// @ingroup html
/// An HTML element that can be rendered to a string. Elements can contain
/// attributes and child nodes.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/html.h>
/// #include <sstream>
/// #include <cassert>
///
/// using namespace sourcemeta::core::html;
///
/// std::ostringstream result;
/// result << div(h1("Title"), p("Content"));
/// assert(result.str() == "<div><h1>Title</h1><p>Content</p></div>");
/// ```
class SOURCEMETA_CORE_HTML_EXPORT HTML {
public:
  HTML(std::string tag, bool self_closing_tag = false)
      : tag_name(std::move(tag)), self_closing(self_closing_tag) {}

  HTML(std::string tag, HTMLAttributes tag_attributes,
       bool self_closing_tag = false)
      : tag_name(std::move(tag)), attributes(std::move(tag_attributes)),
        self_closing(self_closing_tag) {}

  HTML(std::string tag, HTMLAttributes tag_attributes,
       std::vector<HTMLNode> children)
      : tag_name(std::move(tag)), attributes(std::move(tag_attributes)),
        child_elements(std::move(children)), self_closing(false) {}

  HTML(std::string tag, HTMLAttributes tag_attributes,
       std::vector<HTML> children)
      : tag_name(std::move(tag)), attributes(std::move(tag_attributes)),
        self_closing(false) {
    this->child_elements.reserve(children.size());
    for (auto &child_element : children) {
      this->child_elements.emplace_back(std::move(child_element));
    }
  }

  HTML(std::string tag, std::vector<HTMLNode> children)
      : tag_name(std::move(tag)), child_elements(std::move(children)),
        self_closing(false) {}

  HTML(std::string tag, std::vector<HTML> children)
      : tag_name(std::move(tag)), self_closing(false) {
    this->child_elements.reserve(children.size());
    for (auto &child_element : children) {
      this->child_elements.emplace_back(std::move(child_element));
    }
  }

  template <typename... Children>
  HTML(std::string tag, HTMLAttributes tag_attributes, Children &&...children)
      : tag_name(std::move(tag)), attributes(std::move(tag_attributes)),
        self_closing(false) {
    (this->child_elements.push_back(std::forward<Children>(children)), ...);
  }

  template <typename... Children>
  HTML(std::string tag, Children &&...children)
      : tag_name(std::move(tag)), self_closing(false) {
    (this->child_elements.push_back(std::forward<Children>(children)), ...);
  }

  [[nodiscard]] auto render() const -> std::string;

  auto push_back(const HTMLNode &child) -> HTML &;
  auto push_back(HTMLNode &&child) -> HTML &;

  // Stream operator declaration
  friend SOURCEMETA_CORE_HTML_EXPORT auto
  operator<<(std::ostream &output_stream, const HTML &html_element)
      -> std::ostream &;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::string tag_name;
  HTMLAttributes attributes;
  std::vector<HTMLNode> child_elements;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  bool self_closing;

  [[nodiscard]] auto render(const HTMLNode &child_element) const -> std::string;
};

} // namespace sourcemeta::core

#endif
