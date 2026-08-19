#include <sourcemeta/core/jsonl.h>

#include <sourcemeta/core/gzip.h>

#include <istream> // std::basic_istream, std::istream
#include <memory>  // std::make_unique, std::unique_ptr

namespace sourcemeta::core {

struct JSONL::Internal {
  std::unique_ptr<GZIPStreamBuffer> streambuf;
  std::unique_ptr<std::istream> decompressed_stream;
};

JSONL::JSONL(std::basic_istream<JSON::Char, JSON::CharTraits> &input,
             const Mode mode)
    : stream_{&input}, internal_{std::make_unique<Internal>()} {
  if (mode == Mode::GZIP) {
    this->internal_->streambuf = std::make_unique<GZIPStreamBuffer>(input);
    this->internal_->decompressed_stream =
        std::make_unique<std::istream>(this->internal_->streambuf.get());
    this->internal_->decompressed_stream->exceptions(std::istream::badbit);
    this->stream_ = this->internal_->decompressed_stream.get();
  }
}

JSONL::~JSONL() = default;

auto JSONL::begin() -> JSONL::const_iterator { return {this->stream_}; }
auto JSONL::end() -> JSONL::const_iterator { return {nullptr}; }
auto JSONL::cbegin() -> JSONL::const_iterator { return {this->stream_}; }
auto JSONL::cend() -> JSONL::const_iterator { return {nullptr}; }

} // namespace sourcemeta::core
