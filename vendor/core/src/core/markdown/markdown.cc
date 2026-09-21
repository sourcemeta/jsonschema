#include <sourcemeta/core/markdown.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include "blocks.h"
#include "characters.h"
#include "document.h"
#include "inlines.h"
#include "postprocess.h"
#include "render.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint32_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// An input past this size gives up the state of its thread once it is done,
// so that an unusually large conversion does not hold on to the memory it
// needed for the rest of the life of the thread. Inputs this large are far
// past what documentation is made of, so paying for their buffers again is
// better than keeping them around
constexpr std::size_t MAXIMUM_RETAINED_INPUT{0x800000};

// The state of every thread is kept across calls, so that rendering many
// small inputs does not allocate the same buffers over and over
struct MarkdownConverter {
  sourcemeta::core::markdown::Document document;
  sourcemeta::core::markdown::BlockParser blocks{document};
  sourcemeta::core::markdown::InlineParser inlines{document};
  sourcemeta::core::markdown::PostProcessor postprocessor{document};
  sourcemeta::core::markdown::HTMLRenderer renderer{document};
  std::string repaired;
};

// Give up the state of the thread even when the conversion throws
struct ConverterRelease {
  std::optional<MarkdownConverter> &converter;
  bool oversized;
  ~ConverterRelease() {
    if (this->oversized) {
      this->converter.reset();
    }
  }
};

auto replace_invalid_characters(const std::string_view input,
                                std::string &output) -> void {
  output = sourcemeta::core::to_valid_utf8(input);
  if (output.find('\0') != std::string::npos) {
    output = sourcemeta::core::replace(output, std::string_view{"\0", 1},
                                       "\xEF\xBF\xBD");
  }
}

} // namespace

namespace sourcemeta::core {

auto markdown_to_html(const std::string_view input, const bool safe)
    -> std::string {
  thread_local std::optional<MarkdownConverter> state;
  if (!state.has_value()) {
    state.emplace();
  }

  const ConverterRelease release{
      .converter = state, .oversized = input.size() > MAXIMUM_RETAINED_INPUT};
  auto &converter{state.value()};
  converter.document.clear();
  auto source{input};
  // GFM section 2.3 requires replacing the NUL character, and byte sequences
  // that are not well-formed UTF-8 are replaced too
  if (input.find('\0') != std::string_view::npos ||
      !sourcemeta::core::is_valid_utf8(input)) {
    replace_invalid_characters(input, converter.repaired);
    source = converter.repaired;
  }

  // The parser addresses the input through 32-bit offsets
  if (source.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw MarkdownError{"The input is larger than the supported size bound"};
  }

  converter.blocks.parse(source, input.size());
  converter.postprocessor.parse_inlines(converter.inlines);
  converter.postprocessor.process_footnotes();
  return converter.renderer.render(!safe,
                                   source.size() + (source.size() / 2) + 64);
}

} // namespace sourcemeta::core
