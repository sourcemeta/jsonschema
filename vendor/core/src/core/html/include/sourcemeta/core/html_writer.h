#ifndef SOURCEMETA_CORE_HTML_WRITER_H_
#define SOURCEMETA_CORE_HTML_WRITER_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/html_buffer.h>
#include <sourcemeta/core/html_escape.h>
#include <sourcemeta/core/preprocessor.h>

#include <cassert>     // assert
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup html
/// A streaming HTML writer that renders directly to a string buffer.
/// No intermediate DOM tree is built. Elements are serialized as methods
/// are called.
///
/// ```cpp
/// #include <sourcemeta/core/html.h>
/// #include <cassert>
///
/// sourcemeta::core::HTMLWriter document;
/// document.div().attribute("class", "greeting");
/// document.h1("Hello");
/// document.p("World");
/// document.close();
/// ```
class SOURCEMETA_CORE_HTML_EXPORT HTMLWriter {
public:
  /// Pre-allocate the output buffer
  SOURCEMETA_FORCEINLINE inline auto reserve(std::size_t bytes) -> void {
    this->buffer_.reserve(bytes);
  }

  /// Close the most recently opened element
  SOURCEMETA_FORCEINLINE inline auto close() -> HTMLWriter & {
    this->flush_open_tag();
    assert(!this->tag_stack_.empty());
    this->buffer_.append("</");
    this->buffer_.append(this->tag_stack_.back());
    this->buffer_.append(">");
    this->tag_stack_.pop_back();
    return *this;
  }

  /// Add an attribute to the currently open tag. Must be called
  /// immediately after an element method and before any content.
  SOURCEMETA_FORCEINLINE inline auto attribute(std::string_view name,
                                               std::string_view value)
      -> HTMLWriter & {
    assert(this->tag_open_);
    this->buffer_.append(" ");
    this->buffer_.append(name);
    this->buffer_.append("=\"");
    html_escape_append(this->buffer_, value);
    this->buffer_.append("\"");
    return *this;
  }

  /// Write HTML-escaped text content
  SOURCEMETA_FORCEINLINE inline auto text(std::string_view content)
      -> HTMLWriter & {
    this->flush_open_tag();
    html_escape_append(this->buffer_, content);
    return *this;
  }

  /// Write raw HTML content (not escaped)
  SOURCEMETA_FORCEINLINE inline auto raw(std::string_view content)
      -> HTMLWriter & {
    this->flush_open_tag();
    this->buffer_.append(content);
    return *this;
  }

  /// Get the rendered HTML string
  [[nodiscard]] SOURCEMETA_FORCEINLINE inline auto str()
      -> const std::string & {
    this->flush_open_tag();
    return this->buffer_.str();
  }

  /// Write the rendered HTML to an output stream
  auto write(std::ostream &stream) -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

#ifndef DOXYGEN
// Macro to generate container element methods.
// Container elements write <tag> on open and </tag> on close().
// Overloads:
//   .tag()              open with no attributes
//   .tag(text)          open, write escaped text, close (shorthand)
#define HTML_WRITER_CONTAINER(name)                                            \
  SOURCEMETA_FORCEINLINE inline auto name() -> HTMLWriter & {                  \
    this->open_tag(#name);                                                     \
    return *this;                                                              \
  }                                                                            \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                             \
  SOURCEMETA_FORCEINLINE inline auto name(std::string_view text_content)       \
      -> HTMLWriter & {                                                        \
    this->open_tag(#name);                                                     \
    this->text(text_content);                                                  \
    this->close();                                                             \
    return *this;                                                              \
  }

// Same as above but with a different C++ method name than the HTML tag
#define HTML_WRITER_CONTAINER_NAMED(name, tag)                                 \
  SOURCEMETA_FORCEINLINE inline auto name() -> HTMLWriter & {                  \
    this->open_tag(#tag);                                                      \
    return *this;                                                              \
  }                                                                            \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                             \
  SOURCEMETA_FORCEINLINE inline auto name(std::string_view text_content)       \
      -> HTMLWriter & {                                                        \
    this->open_tag(#tag);                                                      \
    this->text(text_content);                                                  \
    this->close();                                                             \
    return *this;                                                              \
  }

// Macro to generate void element methods.
// Void elements are self-closing: <tag /> or <tag attr="val" />
#define HTML_WRITER_VOID(name)                                                 \
  /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                             \
  SOURCEMETA_FORCEINLINE inline auto name() -> HTMLWriter & {                  \
    this->void_tag(#name);                                                     \
    return *this;                                                              \
  }
#endif

  // =========================================================================
  // Document Structure Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(html)
  /// @ingroup html
  HTML_WRITER_VOID(base)
  /// @ingroup html
  HTML_WRITER_CONTAINER(head)
  /// @ingroup html
  HTML_WRITER_VOID(link)
  /// @ingroup html
  HTML_WRITER_VOID(meta)
  /// @ingroup html
  HTML_WRITER_CONTAINER(style)
  /// @ingroup html
  HTML_WRITER_CONTAINER(title)
  /// @ingroup html
  HTML_WRITER_CONTAINER(body)

  // =========================================================================
  // Content Sectioning Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(address)
  /// @ingroup html
  HTML_WRITER_CONTAINER(article)
  /// @ingroup html
  HTML_WRITER_CONTAINER(aside)
  /// @ingroup html
  HTML_WRITER_CONTAINER(footer)
  /// @ingroup html
  HTML_WRITER_CONTAINER(header)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h1)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h2)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h3)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h4)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h5)
  /// @ingroup html
  HTML_WRITER_CONTAINER(h6)
  /// @ingroup html
  HTML_WRITER_CONTAINER(hgroup)
  /// @ingroup html
  HTML_WRITER_CONTAINER(main)
  /// @ingroup html
  HTML_WRITER_CONTAINER(nav)
  /// @ingroup html
  HTML_WRITER_CONTAINER(section)
  /// @ingroup html
  HTML_WRITER_CONTAINER(search)

  // =========================================================================
  // Text Content Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(blockquote)
  /// @ingroup html
  HTML_WRITER_CONTAINER(dd)
  /// @ingroup html
  HTML_WRITER_CONTAINER(div)
  /// @ingroup html
  HTML_WRITER_CONTAINER(dl)
  /// @ingroup html
  HTML_WRITER_CONTAINER(dt)
  /// @ingroup html
  HTML_WRITER_CONTAINER(figcaption)
  /// @ingroup html
  HTML_WRITER_CONTAINER(figure)
  /// @ingroup html
  HTML_WRITER_VOID(hr)
  /// @ingroup html
  HTML_WRITER_CONTAINER(li)
  /// @ingroup html
  HTML_WRITER_CONTAINER(menu)
  /// @ingroup html
  HTML_WRITER_CONTAINER(ol)
  /// @ingroup html
  HTML_WRITER_CONTAINER(p)
  /// @ingroup html
  HTML_WRITER_CONTAINER(pre)
  /// @ingroup html
  HTML_WRITER_CONTAINER(ul)

  // =========================================================================
  // Inline Text Semantics Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(a)
  /// @ingroup html
  HTML_WRITER_CONTAINER(abbr)
  /// @ingroup html
  HTML_WRITER_CONTAINER(b)
  /// @ingroup html
  HTML_WRITER_CONTAINER(bdi)
  /// @ingroup html
  HTML_WRITER_CONTAINER(bdo)
  /// @ingroup html
  HTML_WRITER_VOID(br)
  /// @ingroup html
  HTML_WRITER_CONTAINER(cite)
  /// @ingroup html
  HTML_WRITER_CONTAINER(code)
  /// @ingroup html
  HTML_WRITER_CONTAINER(data)
  /// @ingroup html
  HTML_WRITER_CONTAINER(dfn)
  /// @ingroup html
  HTML_WRITER_CONTAINER(em)
  /// @ingroup html
  HTML_WRITER_CONTAINER(i)
  /// @ingroup html
  HTML_WRITER_CONTAINER(kbd)
  /// @ingroup html
  HTML_WRITER_CONTAINER(mark)
  /// @ingroup html
  HTML_WRITER_CONTAINER(q)
  /// @ingroup html
  HTML_WRITER_CONTAINER(rp)
  /// @ingroup html
  HTML_WRITER_CONTAINER(rt)
  /// @ingroup html
  HTML_WRITER_CONTAINER(ruby)
  /// @ingroup html
  HTML_WRITER_CONTAINER(s)
  /// @ingroup html
  HTML_WRITER_CONTAINER(samp)
  /// @ingroup html
  HTML_WRITER_CONTAINER(small)
  /// @ingroup html
  HTML_WRITER_CONTAINER(span)
  /// @ingroup html
  HTML_WRITER_CONTAINER(strong)
  /// @ingroup html
  HTML_WRITER_CONTAINER(sub)
  /// @ingroup html
  HTML_WRITER_CONTAINER(sup)
  /// @ingroup html
  HTML_WRITER_CONTAINER(time)
  /// @ingroup html
  HTML_WRITER_CONTAINER(u)
  /// @ingroup html
  HTML_WRITER_CONTAINER(var)
  /// @ingroup html
  HTML_WRITER_VOID(wbr)

  // =========================================================================
  // Image and Multimedia Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_VOID(area)
  /// @ingroup html
  HTML_WRITER_CONTAINER(audio)
  /// @ingroup html
  HTML_WRITER_VOID(img)
  /// @ingroup html
  HTML_WRITER_CONTAINER(map)
  /// @ingroup html
  HTML_WRITER_VOID(track)
  /// @ingroup html
  HTML_WRITER_CONTAINER(video)

  // =========================================================================
  // Embedded Content Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_VOID(embed)
  /// @ingroup html
  HTML_WRITER_CONTAINER(iframe)
  /// @ingroup html
  HTML_WRITER_CONTAINER(object)
  /// @ingroup html
  HTML_WRITER_CONTAINER(picture)
  /// @ingroup html
  HTML_WRITER_CONTAINER(portal)
  /// @ingroup html
  HTML_WRITER_VOID(source)

  // =========================================================================
  // Scripting Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(canvas)
  /// @ingroup html
  HTML_WRITER_CONTAINER(noscript)
  /// @ingroup html
  HTML_WRITER_CONTAINER(script)

  // =========================================================================
  // Demarcating Edits Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(del)
  /// @ingroup html
  HTML_WRITER_CONTAINER(ins)

  // =========================================================================
  // Table Content Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(caption)
  /// @ingroup html
  HTML_WRITER_VOID(col)
  /// @ingroup html
  HTML_WRITER_CONTAINER(colgroup)
  /// @ingroup html
  HTML_WRITER_CONTAINER(table)
  /// @ingroup html
  HTML_WRITER_CONTAINER(tbody)
  /// @ingroup html
  HTML_WRITER_CONTAINER(td)
  /// @ingroup html
  HTML_WRITER_CONTAINER(tfoot)
  /// @ingroup html
  HTML_WRITER_CONTAINER(th)
  /// @ingroup html
  HTML_WRITER_CONTAINER(thead)
  /// @ingroup html
  HTML_WRITER_CONTAINER(tr)

  // =========================================================================
  // Forms Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(button)
  /// @ingroup html
  HTML_WRITER_CONTAINER(datalist)
  /// @ingroup html
  HTML_WRITER_CONTAINER(fieldset)
  /// @ingroup html
  HTML_WRITER_CONTAINER(form)
  /// @ingroup html
  HTML_WRITER_VOID(input)
  /// @ingroup html
  HTML_WRITER_CONTAINER(label)
  /// @ingroup html
  HTML_WRITER_CONTAINER(legend)
  /// @ingroup html
  HTML_WRITER_CONTAINER(meter)
  /// @ingroup html
  HTML_WRITER_CONTAINER(optgroup)
  /// @ingroup html
  HTML_WRITER_CONTAINER(option)
  /// @ingroup html
  HTML_WRITER_CONTAINER(output)
  /// @ingroup html
  HTML_WRITER_CONTAINER(progress)
  /// @ingroup html
  HTML_WRITER_CONTAINER(select)
  /// @ingroup html
  HTML_WRITER_CONTAINER(textarea)

  // =========================================================================
  // Interactive Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(details)
  /// @ingroup html
  HTML_WRITER_CONTAINER(dialog)
  /// @ingroup html
  HTML_WRITER_CONTAINER(summary)

  // =========================================================================
  // Web Components Elements
  // =========================================================================

  /// @ingroup html
  HTML_WRITER_CONTAINER(slot)
  /// @ingroup html
  HTML_WRITER_CONTAINER_NAMED(template_, template)

#ifndef DOXYGEN
#undef HTML_WRITER_CONTAINER
#undef HTML_WRITER_CONTAINER_NAMED
#undef HTML_WRITER_VOID
#endif

private:
  SOURCEMETA_FORCEINLINE inline auto open_tag(std::string_view tag) -> void {
    this->flush_open_tag();
    this->buffer_.append("<");
    this->buffer_.append(tag);
    this->tag_stack_.push_back(tag);
    this->tag_open_ = true;
    this->tag_open_is_void_ = false;
  }

  SOURCEMETA_FORCEINLINE inline auto void_tag(std::string_view tag) -> void {
    this->flush_open_tag();
    this->buffer_.append("<");
    this->buffer_.append(tag);
    this->tag_open_ = true;
    this->tag_open_is_void_ = true;
  }

  auto flush_open_tag() -> void;

  HTMLBuffer buffer_;
  std::vector<std::string_view> tag_stack_;
  bool tag_open_{false};
  bool tag_open_is_void_{false};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
