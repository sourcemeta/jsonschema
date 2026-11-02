#ifndef SOURCEMETA_CORE_JSONSCHEMA_HASHER_H
#define SOURCEMETA_CORE_JSONSCHEMA_HASHER_H

#include <sourcemeta/core/jsonpointer.h>

#include <cstddef> // std::size_t

namespace sourcemeta::core {

// Framing keys several of its own tables by the position a subschema sits at,
// and it inserts one entry per subschema, so how those positions spread across
// buckets decides what framing a large document costs.
//
// The hash a position carries by default reads three of its tokens, the first,
// the middle and the last, and adds them together. That is a deliberate
// constant cost, which the deeply nested schemas this library is also asked to
// frame depend on, and for a schema whose positions differ near their end it
// spreads them well.
//
// It is close to the worst case for a description whose shape a specification
// fixes. Where every position reads as a path, a method, a container and a
// member of it, the first token, the middle one and the last are the ones the
// shape holds constant, and the ones that tell two positions apart are the
// second and the second to last. Thousands of positions then hash alike, and
// an insertion into the one bucket holding them compares against everything
// already there, which turns the cost of framing such a description into the
// square of how many positions it has.
//
// So this reads the second and the second to last in place of the middle,
// keeps to four tokens however deep the position is, and folds each one in
// rather than adding it, so that two positions holding the same tokens in a
// different arrangement still part. A position shorter than that is read to
// its end and no further, with nothing read twice.
//
// This belongs to framing rather than to positions in general: it is the shape
// of what framing is handed that makes the default a poor fit, and the default
// serves every other caller well
struct PositionHasher {
  [[nodiscard]] static auto token_of(const WeakPointer &position,
                                     const std::size_t index) noexcept
      -> std::size_t {
    const auto &token{position.at(index)};
    if (!token.is_property()) {
      return static_cast<std::size_t>(token.to_index());
    }

    // The first half of a property hash is the half the default one folds too,
    // which carries entropy enough to tell properties apart
    const auto property{token.property_hash().a};
    return static_cast<std::size_t>(property) ^
           static_cast<std::size_t>(property >> 64);
  }

  [[nodiscard]] static auto fold(const std::size_t accumulated,
                                 const std::size_t value) noexcept
      -> std::size_t {
    return accumulated ^ (value + 0x9e3779b97f4a7c15ULL + (accumulated << 6) +
                          (accumulated >> 2));
  }

  [[nodiscard]] auto operator()(const WeakPointer &position) const noexcept
      -> std::size_t {
    const auto size{position.size()};
    if (size == 0) {
      return 0;
    }

    auto result{fold(size, token_of(position, 0))};
    if (size > 1) {
      result = fold(result, token_of(position, size - 1));
    }

    if (size > 2) {
      result = fold(result, token_of(position, 1));
    }

    if (size > 3) {
      result = fold(result, token_of(position, size - 2));
    }

    return result;
  }
};

} // namespace sourcemeta::core

#endif
