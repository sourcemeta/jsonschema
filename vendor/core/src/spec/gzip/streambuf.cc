#include <sourcemeta/core/gzip.h>

#include "inflate.h"

#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <cstring> // std::memmove
#include <ios>     // std::streamsize
#include <istream> // std::istream
#include <vector>  // std::vector

namespace sourcemeta::core {

static constexpr std::size_t GZIP_INPUT_BUFFER_SIZE{65536};
static constexpr std::size_t GZIP_OUTPUT_BUFFER_SIZE{262144};
// RFC 1951 section 3.2.5 caps the distance of a back-reference
static constexpr std::size_t GZIP_HISTORY_SIZE{32768};

struct GZIPStreamBuffer::Internal {
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  std::istream &stream;
  InflateDecoder decoder;
  std::vector<std::uint8_t> input;
  std::vector<std::uint8_t> output;
  std::size_t input_next{0};
  std::size_t input_end{0};
  std::size_t output_next{0};
  bool input_final{false};
  bool stream_ended{false};

  Internal(std::istream &source)
      : stream{source}, input(GZIP_INPUT_BUFFER_SIZE),
        output(GZIP_OUTPUT_BUFFER_SIZE) {}

  // The bytes the decoder already moved into its bit buffer stay in place, as
  // the decoder may hand them back to the input
  auto refill() -> void {
    const auto keep{this->input_next - this->decoder.buffered_input()};
    std::memmove(this->input.data(), this->input.data() + keep,
                 this->input_end - keep);
    this->input_end -= keep;
    this->input_next -= keep;
    this->stream.read(
        reinterpret_cast<char *>(this->input.data() + this->input_end),
        static_cast<std::streamsize>(GZIP_INPUT_BUFFER_SIZE - this->input_end));
    this->input_end += static_cast<std::size_t>(this->stream.gcount());
    if (!this->stream) {
      this->input_final = true;
    }
  }
};

GZIPStreamBuffer::GZIPStreamBuffer(std::istream &compressed_stream)
    : internal_{new Internal{compressed_stream}} {}

GZIPStreamBuffer::~GZIPStreamBuffer() = default;

auto GZIPStreamBuffer::underflow() -> int_type {
  if ((this->gptr() != nullptr) && this->gptr() < this->egptr()) {
    return traits_type::to_int_type(*this->gptr());
  }

  auto &internal{*this->internal_};
  while (!internal.stream_ended) {
    if (GZIP_OUTPUT_BUFFER_SIZE - internal.output_next < GZIP_HISTORY_SIZE) {
      std::memmove(internal.output.data(),
                   internal.output.data() + internal.output_next -
                       GZIP_HISTORY_SIZE,
                   GZIP_HISTORY_SIZE);
      internal.output_next = GZIP_HISTORY_SIZE;
    }

    const auto produced_start{internal.output_next};
    InflateBuffers buffers{
        .input_next = internal.input.data() + internal.input_next,
        .input_end = internal.input.data() + internal.input_end,
        .input_final = internal.input_final,
        .output_begin = internal.output.data(),
        .output_next = internal.output.data() + internal.output_next,
        .output_end = internal.output.data() + GZIP_OUTPUT_BUFFER_SIZE};
    const auto status{internal.decoder.decode(buffers)};
    internal.input_next =
        static_cast<std::size_t>(buffers.input_next - internal.input.data());
    internal.output_next =
        static_cast<std::size_t>(buffers.output_next - internal.output.data());
    if (status == InflateStatus::Done) {
      internal.stream_ended = true;
    } else if (status == InflateStatus::NeedInput) {
      internal.refill();
    }

    if (internal.output_next > produced_start) {
      auto *const start{
          reinterpret_cast<char *>(internal.output.data() + produced_start)};
      this->setg(start, start,
                 reinterpret_cast<char *>(internal.output.data() +
                                          internal.output_next));
      return traits_type::to_int_type(*this->gptr());
    }
  }

  return traits_type::eof();
}

} // namespace sourcemeta::core
