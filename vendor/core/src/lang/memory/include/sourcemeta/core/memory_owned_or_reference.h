#ifndef SOURCEMETA_CORE_MEMORY_OWNED_OR_REFERENCE_H_
#define SOURCEMETA_CORE_MEMORY_OWNED_OR_REFERENCE_H_

#include <cassert>     // assert
#include <concepts>    // std::move_constructible, std::copy_constructible
#include <memory>      // std::addressof
#include <optional>    // std::optional, std::nullopt_t
#include <type_traits> // std::is_object_v
#include <utility>     // std::move

namespace sourcemeta::core {

/// @ingroup memory
/// What a value that can be either owned or referred to must satisfy
template <typename T>
concept Ownable = std::is_object_v<T> && std::move_constructible<T>;

/// @ingroup memory
/// Either a value this holds itself, a reference to one that outlives it, or
/// nothing at all. For example:
///
/// ```cpp
/// #include <sourcemeta/core/memory.h>
/// #include <cassert>
/// #include <string>
///
/// static const std::string CACHED{"foo"};
///
/// const sourcemeta::core::OwnedOrReference<std::string> reference{CACHED};
/// assert(&reference.value() == &CACHED);
///
/// const sourcemeta::core::OwnedOrReference<std::string> owned{
///     std::string{"bar"}};
/// assert(owned.value() == "bar");
/// ```
///
/// Reach for this when a function sometimes materialises its result and
/// sometimes hands back something it already has. Producers that build a
/// value, by reading a file, performing a network request, or computing it,
/// return it as they always would. Producers backed by storage that outlives
/// the call, such as a long lived cache, return a reference instead and skip
/// the copy.
///
/// A reference must stay put and stay alive for as long as the consumer reads
/// it. Anything temporary binds to the owning constructor, so a temporary can
/// never be captured by reference here. A value that another one of these owns
/// does not qualify either, as assigning to that one destroys what it holds.
template <Ownable T> class OwnedOrReference {
public:
  /// Hold nothing
  OwnedOrReference() = default;

  /// Hold nothing
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  OwnedOrReference([[maybe_unused]] const std::nullopt_t value) {}

  /// Take ownership of a value that may or may not be there
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  OwnedOrReference(std::optional<T> &&value) : owned_{std::move(value)} {}

  /// Take ownership of a value
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  OwnedOrReference(T &&value) : owned_{std::move(value)} {}

  /// Refer to a value that outlives this. Anything temporary binds to the
  /// owning constructor above instead, so this never refers to a dead value
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  OwnedOrReference(const T &value) : referenced_{std::addressof(value)} {}

  // A constant temporary would otherwise bind to the referencing constructor
  // and leave a dangling pointer behind
  OwnedOrReference(const T &&value) = delete;

  // Prevent accidental copies, as copying is the very thing this type exists
  // to avoid. Take ownership through `to_owned` instead
  OwnedOrReference(const OwnedOrReference &) = delete;
  auto operator=(const OwnedOrReference &) -> OwnedOrReference & = delete;
  /// Move
  OwnedOrReference(OwnedOrReference &&) = default;
  /// Move
  auto operator=(OwnedOrReference &&) -> OwnedOrReference & = default;
  ~OwnedOrReference() = default;

  /// Whether there is anything to read
  [[nodiscard]] auto has_value() const noexcept -> bool {
    return this->referenced_ != nullptr || this->owned_.has_value();
  }

  /// Read the value, however it is held
  [[nodiscard]] auto value() const -> const T & {
    assert(this->has_value());
    return this->referenced_ != nullptr ? *this->referenced_
                                        : this->owned_.value();
  }

  /// Read the value, however it is held
  [[nodiscard]] auto operator*() const -> const T & { return this->value(); }

  /// Read the value, however it is held
  [[nodiscard]] auto operator->() const -> const T * {
    return std::addressof(this->value());
  }

  /// Get a value the caller owns, moving out of this one when it owns it and
  /// copying only when it holds a reference
  [[nodiscard]] auto to_owned() && -> T
    requires std::copy_constructible<T>
  {
    assert(this->has_value());
    if (this->referenced_ != nullptr) {
      return *this->referenced_;
    }

    return std::move(this->owned_).value();
  }

private:
  std::optional<T> owned_;
  const T *referenced_{nullptr};
};

} // namespace sourcemeta::core

#endif
