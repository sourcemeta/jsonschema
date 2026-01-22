#ifndef SOURCEMETA_CORE_HTML_ELEMENTS_H_
#define SOURCEMETA_CORE_HTML_ELEMENTS_H_

#include <sourcemeta/core/html_encoder.h>

namespace sourcemeta::core::html {

#ifndef DOXYGEN
#define HTML_VOID_ELEMENT(name)                                                \
  inline auto name() -> HTML { return HTML(#name, true); }                     \
  inline auto name(HTMLAttributes attributes) -> HTML {                        \
    return HTML(#name, std::move(attributes), true);                           \
  }

#define HTML_CONTAINER_ELEMENT_NAMED(name, tag)                                \
  inline auto name(HTMLAttributes attributes) -> HTML {                        \
    return HTML(#tag, std::move(attributes));                                  \
  }                                                                            \
  template <typename... Children>                                              \
  inline auto name(HTMLAttributes attributes, Children &&...children)          \
      -> HTML {                                                                \
    return HTML(#tag, std::move(attributes),                                   \
                std::forward<Children>(children)...);                          \
  }                                                                            \
  template <typename... Children>                                              \
  inline auto name(Children &&...children) -> HTML {                           \
    return HTML(#tag, std::forward<Children>(children)...);                    \
  }

#define HTML_CONTAINER_ELEMENT(name) HTML_CONTAINER_ELEMENT_NAMED(name, name)

#define HTML_COMPACT_ELEMENT(name)                                             \
  inline auto name(HTMLAttributes attributes) -> HTML {                        \
    return HTML(#name, std::move(attributes));                                 \
  }                                                                            \
  template <typename... Children>                                              \
  inline auto name(HTMLAttributes attributes, Children &&...children)          \
      -> HTML {                                                                \
    return HTML(#name, std::move(attributes),                                  \
                std::forward<Children>(children)...);                          \
  }                                                                            \
  template <typename... Children>                                              \
  inline auto name(Children &&...children) -> HTML {                           \
    return HTML(#name, std::forward<Children>(children)...);                   \
  }

#define HTML_VOID_ATTR_ELEMENT(name)                                           \
  inline auto name(HTMLAttributes attributes) -> HTML {                        \
    return HTML(#name, std::move(attributes), true);                           \
  }
#endif

/// @ingroup html
inline auto raw(std::string html_content) -> HTMLRaw {
  return HTMLRaw{std::move(html_content)};
}

// =============================================================================
// Document Structure Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(html)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(base)

/// @ingroup html
HTML_CONTAINER_ELEMENT(head)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(link)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(meta)

/// @ingroup html
HTML_CONTAINER_ELEMENT(style)

/// @ingroup html
HTML_CONTAINER_ELEMENT(title)

/// @ingroup html
HTML_CONTAINER_ELEMENT(body)

// =============================================================================
// Content Sectioning Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(address)

/// @ingroup html
HTML_CONTAINER_ELEMENT(article)

/// @ingroup html
HTML_CONTAINER_ELEMENT(aside)

/// @ingroup html
HTML_CONTAINER_ELEMENT(footer)

/// @ingroup html
HTML_CONTAINER_ELEMENT(header)

/// @ingroup html
HTML_COMPACT_ELEMENT(h1)
/// @ingroup html
HTML_COMPACT_ELEMENT(h2)
/// @ingroup html
HTML_COMPACT_ELEMENT(h3)
/// @ingroup html
HTML_COMPACT_ELEMENT(h4)
/// @ingroup html
HTML_COMPACT_ELEMENT(h5)
/// @ingroup html
HTML_COMPACT_ELEMENT(h6)

/// @ingroup html
HTML_CONTAINER_ELEMENT(hgroup)

/// @ingroup html
HTML_CONTAINER_ELEMENT(main)

/// @ingroup html
HTML_CONTAINER_ELEMENT(nav)

/// @ingroup html
HTML_CONTAINER_ELEMENT(section)

/// @ingroup html
HTML_CONTAINER_ELEMENT(search)

// =============================================================================
// Text Content Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(blockquote)

/// @ingroup html
HTML_COMPACT_ELEMENT(dd)

/// @ingroup html
HTML_CONTAINER_ELEMENT(div)

/// @ingroup html
HTML_COMPACT_ELEMENT(dl)

/// @ingroup html
HTML_COMPACT_ELEMENT(dt)

/// @ingroup html
HTML_CONTAINER_ELEMENT(figcaption)

/// @ingroup html
HTML_CONTAINER_ELEMENT(figure)

/// @ingroup html
HTML_VOID_ELEMENT(hr)

/// @ingroup html
HTML_COMPACT_ELEMENT(li)

/// @ingroup html
HTML_CONTAINER_ELEMENT(menu)

/// @ingroup html
HTML_COMPACT_ELEMENT(ol)

/// @ingroup html
HTML_COMPACT_ELEMENT(p)

/// @ingroup html
HTML_CONTAINER_ELEMENT(pre)

/// @ingroup html
HTML_CONTAINER_ELEMENT(ul)

// =============================================================================
// Inline Text Semantics Elements
// =============================================================================

/// @ingroup html
HTML_COMPACT_ELEMENT(a)

/// @ingroup html
HTML_CONTAINER_ELEMENT(abbr)

/// @ingroup html
HTML_COMPACT_ELEMENT(b)

/// @ingroup html
HTML_CONTAINER_ELEMENT(bdi)

/// @ingroup html
HTML_CONTAINER_ELEMENT(bdo)

/// @ingroup html
HTML_VOID_ELEMENT(br)

/// @ingroup html
HTML_CONTAINER_ELEMENT(cite)

/// @ingroup html
HTML_CONTAINER_ELEMENT(code)

/// @ingroup html
HTML_CONTAINER_ELEMENT(data)

/// @ingroup html
HTML_CONTAINER_ELEMENT(dfn)

/// @ingroup html
HTML_COMPACT_ELEMENT(em)

/// @ingroup html
HTML_COMPACT_ELEMENT(i)

/// @ingroup html
HTML_CONTAINER_ELEMENT(kbd)

/// @ingroup html
HTML_CONTAINER_ELEMENT(mark)

/// @ingroup html
HTML_COMPACT_ELEMENT(q)

/// @ingroup html
HTML_COMPACT_ELEMENT(rp)

/// @ingroup html
HTML_COMPACT_ELEMENT(rt)

/// @ingroup html
HTML_CONTAINER_ELEMENT(ruby)

/// @ingroup html
HTML_COMPACT_ELEMENT(s)

/// @ingroup html
HTML_CONTAINER_ELEMENT(samp)

/// @ingroup html
HTML_CONTAINER_ELEMENT(small)

/// @ingroup html
HTML_CONTAINER_ELEMENT(span)

/// @ingroup html
HTML_CONTAINER_ELEMENT(strong)

/// @ingroup html
HTML_CONTAINER_ELEMENT(sub)

/// @ingroup html
HTML_CONTAINER_ELEMENT(sup)

/// @ingroup html
HTML_CONTAINER_ELEMENT(time)

/// @ingroup html
HTML_COMPACT_ELEMENT(u)

/// @ingroup html
HTML_CONTAINER_ELEMENT(var)

/// @ingroup html
HTML_VOID_ELEMENT(wbr)

// =============================================================================
// Image and Multimedia Elements
// =============================================================================

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(area)

/// @ingroup html
HTML_CONTAINER_ELEMENT(audio)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(img)

/// @ingroup html
HTML_CONTAINER_ELEMENT(map)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(track)

/// @ingroup html
HTML_CONTAINER_ELEMENT(video)

// =============================================================================
// Embedded Content Elements
// =============================================================================

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(embed)

/// @ingroup html
HTML_CONTAINER_ELEMENT(iframe)

/// @ingroup html
HTML_CONTAINER_ELEMENT(object)

/// @ingroup html
HTML_CONTAINER_ELEMENT(picture)

/// @ingroup html
HTML_CONTAINER_ELEMENT(portal)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(source)

// =============================================================================
// Scripting Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(canvas)

/// @ingroup html
HTML_CONTAINER_ELEMENT(noscript)

/// @ingroup html
HTML_CONTAINER_ELEMENT(script)

// =============================================================================
// Demarcating Edits Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(del)

/// @ingroup html
HTML_CONTAINER_ELEMENT(ins)

// =============================================================================
// Table Content Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(caption)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(col)

/// @ingroup html
HTML_CONTAINER_ELEMENT(colgroup)

/// @ingroup html
HTML_CONTAINER_ELEMENT(table)

/// @ingroup html
HTML_CONTAINER_ELEMENT(tbody)

/// @ingroup html
HTML_COMPACT_ELEMENT(td)

/// @ingroup html
HTML_CONTAINER_ELEMENT(tfoot)

/// @ingroup html
HTML_COMPACT_ELEMENT(th)

/// @ingroup html
HTML_CONTAINER_ELEMENT(thead)

/// @ingroup html
HTML_CONTAINER_ELEMENT(tr)

// =============================================================================
// Forms Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(button)

/// @ingroup html
HTML_CONTAINER_ELEMENT(datalist)

/// @ingroup html
HTML_CONTAINER_ELEMENT(fieldset)

/// @ingroup html
HTML_CONTAINER_ELEMENT(form)

/// @ingroup html
HTML_VOID_ATTR_ELEMENT(input)

/// @ingroup html
HTML_CONTAINER_ELEMENT(label)

/// @ingroup html
HTML_CONTAINER_ELEMENT(legend)

/// @ingroup html
HTML_CONTAINER_ELEMENT(meter)

/// @ingroup html
HTML_CONTAINER_ELEMENT(optgroup)

/// @ingroup html
HTML_CONTAINER_ELEMENT(option)

/// @ingroup html
HTML_CONTAINER_ELEMENT(output)

/// @ingroup html
HTML_CONTAINER_ELEMENT(progress)

/// @ingroup html
HTML_CONTAINER_ELEMENT(select)

/// @ingroup html
HTML_CONTAINER_ELEMENT(textarea)

// =============================================================================
// Interactive Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(details)

/// @ingroup html
HTML_CONTAINER_ELEMENT(dialog)

/// @ingroup html
HTML_CONTAINER_ELEMENT(summary)

// =============================================================================
// Web Components Elements
// =============================================================================

/// @ingroup html
HTML_CONTAINER_ELEMENT(slot)

/// @ingroup html
HTML_CONTAINER_ELEMENT_NAMED(template_, template)

#ifndef DOXYGEN
#undef HTML_VOID_ELEMENT
#undef HTML_CONTAINER_ELEMENT
#undef HTML_CONTAINER_ELEMENT_NAMED
#undef HTML_COMPACT_ELEMENT
#undef HTML_VOID_ATTR_ELEMENT
#endif

} // namespace sourcemeta::core::html

#endif
