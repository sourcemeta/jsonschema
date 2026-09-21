#ifndef SOURCEMETA_CORE_HTML_WRITER_H_
#define SOURCEMETA_CORE_HTML_WRITER_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/html_buffer.h>
#include <sourcemeta/core/html_escape.h>
#include <sourcemeta/core/preprocessor.h>

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string>      // std::string
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
  SOURCEMETA_FORCEINLINE auto reserve(std::size_t bytes) -> void {
    this->buffer_.reserve(bytes);
  }

  /// Close the most recently opened element. Closing when no element is open
  /// has no effect.
  SOURCEMETA_FORCEINLINE auto close() -> HTMLWriter & {
    this->tag_open_ = false;
    assert(!this->tag_stack_.empty());
    if (this->tag_stack_.empty()) [[unlikely]] {
      return *this;
    }
    const auto &closing{this->tag_stack_.back()};
    // Copying every byte of the fixed storage is cheaper than copying a
    // variable number of them, and the bytes past the tag are then dropped
    this->buffer_.reserve_additional(closing.bytes.size());
    this->buffer_.append_unchecked(
        std::string_view{closing.bytes.data(), closing.bytes.size()});
    this->buffer_.remove_suffix(closing.bytes.size() - closing.size);
    this->tag_stack_.pop_back();
    return *this;
  }

  /// Add an attribute to the currently open tag. Must be called
  /// immediately after an element method and before any content.
  SOURCEMETA_FORCEINLINE auto attribute(std::string_view name,
                                        std::string_view value)
      -> HTMLWriter & {
    assert(this->tag_open_);
    this->reopen_tag();
    // The space before the name, the equals sign and the quotation mark after
    // it, along with the end of the tag, which takes four bytes for a void
    // element
    this->buffer_.reserve_additional(name.size() + 7);
    this->buffer_.append_unchecked(" ");
    this->buffer_.append_unchecked(name);
    this->buffer_.append_unchecked("=\"");
    html_escape_append(this->buffer_, value);
    this->buffer_.reserve_additional(4);
    if (this->tag_open_is_void_) {
      this->buffer_.append_unchecked("\" />");
    } else {
      this->buffer_.append_unchecked("\">");
    }
    return *this;
  }

  /// Add an attribute without a value, such as a boolean attribute, to the
  /// currently open tag. Must be called immediately after an element method
  /// and before any content. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/html.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::HTMLWriter document;
  /// document.input().attribute("type", "checkbox").attribute("checked");
  /// assert(document.str() == "<input type=\"checkbox\" checked />");
  /// ```
  SOURCEMETA_FORCEINLINE auto attribute(std::string_view name) -> HTMLWriter & {
    assert(this->tag_open_);
    this->reopen_tag();
    this->buffer_.reserve_additional(name.size() + 1);
    this->buffer_.append_unchecked(" ");
    this->buffer_.append_unchecked(name);
    this->end_open_tag();
    return *this;
  }

  /// Write a line feed unless nothing was written yet or the output already
  /// ends with one, so that the markup that follows starts on a line of its
  /// own. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/html.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::HTMLWriter document;
  /// document.p("Hello");
  /// document.ensure_line_feed();
  /// document.ensure_line_feed();
  /// document.p("World");
  /// assert(document.str() == "<p>Hello</p>\n<p>World</p>");
  /// ```
  SOURCEMETA_FORCEINLINE auto ensure_line_feed() -> HTMLWriter & {
    this->tag_open_ = false;
    if (this->buffer_.size() > 0 && this->buffer_.back() != '\n') {
      this->buffer_.append('\n');
    }

    return *this;
  }

  /// Move the rendered HTML string out of the writer, leaving the writer empty
  /// and ready to render another document. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/html.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::HTMLWriter document;
  /// document.p("Hello");
  /// const auto result{document.take()};
  /// assert(result == "<p>Hello</p>");
  /// assert(document.str().empty());
  /// ```
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto take() -> std::string {
    this->tag_open_ = false;
    this->tag_stack_.clear();
    return this->buffer_.take();
  }

  /// Discard the output and every open element. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/html.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::HTMLWriter document;
  /// document.div().p("Hello");
  /// document.clear();
  /// assert(document.str().empty());
  /// ```
  SOURCEMETA_FORCEINLINE auto clear() -> void {
    this->tag_open_ = false;
    this->tag_stack_.clear();
    this->buffer_.clear();
  }

  /// Write HTML-escaped text content. The single-argument element shorthand
  /// routes through this and therefore also escapes. The HTML serialization
  /// emits the content of a raw-text element literally, so escaping its content
  /// would corrupt it. This writer does not special-case content by element, so
  /// the content of a raw-text element must be written unescaped rather than as
  /// escaped text, and it must not contain that element's closing-tag sequence.
  SOURCEMETA_FORCEINLINE auto text(std::string_view content) -> HTMLWriter & {
    this->tag_open_ = false;
    html_escape_append(this->buffer_, content);
    return *this;
  }

  /// Write content without HTML-escaping. This is how the content of a raw-text
  /// element is emitted, since escaped text would corrupt it.
  SOURCEMETA_FORCEINLINE auto raw(std::string_view content) -> HTMLWriter & {
    this->tag_open_ = false;
    this->buffer_.append(content);
    return *this;
  }

  /// Get the rendered HTML string
  [[nodiscard]] SOURCEMETA_FORCEINLINE auto str() -> const std::string & {
    return this->buffer_.str();
  }

  /// Write the rendered HTML to an output stream
  auto write(std::ostream &stream) -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

#ifndef DOXYGEN
// Macro to generate container element methods.
// Container elements write <tag> on open and </tag> on close().
// Overloads:
//   .tag()              open with no attributes
//   .tag(text)          open, write escaped text, close (shorthand)
// NOLINTBEGIN(bugprone-macro-parentheses)
#define HTML_WRITER_CONTAINER(name)                                            \
  SOURCEMETA_FORCEINLINE auto name() -> HTMLWriter & {                         \
    this->open_tag("<" #name ">", ClosingTag{"</" #name ">"});                 \
    return *this;                                                              \
  }                                                                            \
  SOURCEMETA_FORCEINLINE auto name(std::string_view text_content)              \
      -> HTMLWriter & {                                                        \
    this->open_tag("<" #name ">", ClosingTag{"</" #name ">"});                 \
    this->text(text_content);                                                  \
    this->close();                                                             \
    return *this;                                                              \
  }
// NOLINTEND(bugprone-macro-parentheses)

// Same as above but with a different C++ method name than the HTML tag
// NOLINTBEGIN(bugprone-macro-parentheses)
#define HTML_WRITER_CONTAINER_NAMED(name, tag)                                 \
  SOURCEMETA_FORCEINLINE auto name() -> HTMLWriter & {                         \
    this->open_tag("<" #tag ">", ClosingTag{"</" #tag ">"});                   \
    return *this;                                                              \
  }                                                                            \
  SOURCEMETA_FORCEINLINE auto name(std::string_view text_content)              \
      -> HTMLWriter & {                                                        \
    this->open_tag("<" #tag ">", ClosingTag{"</" #tag ">"});                   \
    this->text(text_content);                                                  \
    this->close();                                                             \
    return *this;                                                              \
  }
// NOLINTEND(bugprone-macro-parentheses)

// Macro to generate void element methods.
// Void elements are self-closing: <tag /> or <tag attr="val" />
// NOLINTBEGIN(bugprone-macro-parentheses)
#define HTML_WRITER_VOID(name)                                                 \
  SOURCEMETA_FORCEINLINE auto name() -> HTMLWriter & {                         \
    this->void_tag("<" #name " />");                                           \
    return *this;                                                              \
  }
// NOLINTEND(bugprone-macro-parentheses)
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
  // The trailing underscore keeps this element name from colliding with the
  // template keyword
  // NOLINTNEXTLINE(readability-identifier-naming)
  HTML_WRITER_CONTAINER_NAMED(template_, template)

#ifndef DOXYGEN
#undef HTML_WRITER_CONTAINER
#undef HTML_WRITER_CONTAINER_NAMED
#undef HTML_WRITER_VOID
#endif

private:
  // A closing tag kept in a fixed number of bytes, as copying a constant
  // number of bytes is cheaper than copying a variable one
  struct ClosingTag {
    constexpr explicit ClosingTag(const std::string_view tag) noexcept
        : size{static_cast<std::uint8_t>(tag.size())} {
      for (std::size_t index = 0; index < tag.size(); index += 1) {
        this->bytes[index] = tag[index];
      }
    }

    std::array<char, 16> bytes{};
    std::uint8_t size;
  };

  SOURCEMETA_FORCEINLINE auto open_tag(std::string_view opening,
                                       const ClosingTag &closing) -> void {
    this->buffer_.append(opening);
    this->tag_stack_.push_back(closing);
    this->tag_open_ = true;
    this->tag_open_is_void_ = false;
  }

  SOURCEMETA_FORCEINLINE auto void_tag(std::string_view opening) -> void {
    this->buffer_.append(opening);
    this->tag_open_ = true;
    this->tag_open_is_void_ = true;
  }

  // Remove the characters that end the tag opened last, so that attributes
  // can go before them
  SOURCEMETA_FORCEINLINE auto reopen_tag() -> void {
    this->buffer_.remove_suffix(this->tag_open_is_void_ ? 3 : 1);
  }

  SOURCEMETA_FORCEINLINE auto end_open_tag() -> void {
    if (this->tag_open_is_void_) {
      this->buffer_.append(" />");
    } else {
      this->buffer_.append('>');
    }
  }

  HTMLBuffer buffer_;
  std::vector<ClosingTag> tag_stack_;
  bool tag_open_{false};
  bool tag_open_is_void_{false};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

} // namespace sourcemeta::core

#endif
