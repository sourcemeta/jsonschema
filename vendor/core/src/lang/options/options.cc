#include <sourcemeta/core/options.h>

#include <cassert> // assert
#include <cstddef> // std::size_t
#include <utility> // std::forward

namespace {

template <typename T, typename V>
auto emplace_back_unique(T &container, V &&element) -> const auto & {
  return *(container.emplace_back(
      std::make_unique<typename T::value_type::element_type>(
          std::forward<V>(element))));
}

template <typename T>
auto find_canonical_name(const T &aliases, const typename T::key_type &alias)
    -> const T::mapped_type & {
  const auto iterator{aliases.find(alias)};
  if (iterator == aliases.cend()) {
    throw sourcemeta::core::OptionsUnknownOptionError(alias);
  }
  return iterator->second;
}

} // namespace

namespace sourcemeta::core {

const std::vector<std::string_view> Options::EMPTY = {};

auto Options::option(std::string &&name,
                     std::initializer_list<std::string> aliases) -> void {
  assert(!name.empty());
  const std::string_view view{emplace_back_unique(this->storage_, name)};
  this->aliases_.try_emplace(view, view);
  for (const auto &alias : aliases) {
    assert(!alias.empty());
    const std::string_view alias_view{
        emplace_back_unique(this->storage_, alias)};
    this->aliases_.try_emplace(alias_view, view);
  }
}

auto Options::flag(std::string &&name,
                   std::initializer_list<std::string> aliases) -> void {
  assert(!name.empty());
  const std::string_view view{emplace_back_unique(this->storage_, name)};
  this->aliases_.try_emplace(view, view);
  for (const auto &alias : aliases) {
    assert(!alias.empty());
    const std::string_view alias_view{
        emplace_back_unique(this->storage_, alias)};
    this->aliases_.try_emplace(alias_view, view);
  }

  this->flags_.emplace(view);
}

auto Options::at(const std::string_view name) const
    -> const std::vector<std::string_view> & {
  assert(!name.empty());
  const auto iterator{this->options_.find(name)};
  return iterator == this->options_.cend() ? Options::EMPTY : iterator->second;
}

auto Options::contains(const std::string_view name) const -> bool {
  return this->options_.contains(name);
}

auto Options::positional() const -> const std::vector<std::string_view> & {
  const auto iterator{this->options_.find(POSITIONAL_ARGUMENT_NAME)};
  return iterator == this->options_.cend() ? Options::EMPTY : iterator->second;
}

auto Options::parse(const int argc,
                    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
                    const char *const argv[], const OptionsModifiers options)
    -> void {
  bool end_of_options{false};
  // We assume that the first argument is the program name
  const auto argument_count{static_cast<std::size_t>(argc)};
  for (std::size_t index{options.skip + 1}; index < argument_count; index++) {
    const std::string_view token{argv[index]};

    if (end_of_options) {
      this->options_[POSITIONAL_ARGUMENT_NAME].emplace_back(token);
      continue;
    }
    if (token == "--") {
      end_of_options = true;
      continue;
    }

    const auto *const next{(index + 1) < argument_count ? argv[index + 1]
                                                        : nullptr};

    // Parse long options
    if (token.size() >= 3 && token[0] == '-' && token[1] == '-') {
      const auto separator{token.find('=')};
      const auto name{(separator == std::string_view::npos)
                          ? token.substr(2)
                          : token.substr(2, separator - 2)};
      const auto &canonical{find_canonical_name(this->aliases_, name)};
      const auto is_flag{this->flags_.contains(canonical)};

      if (is_flag) {
        if (separator == std::string_view::npos) {
          this->options_[canonical].push_back(token.substr(2));
        } else {
          throw OptionsUnexpectedValueFlagError(name);
        }
      } else if (separator != std::string_view::npos) {
        this->options_[canonical].push_back(token.substr(separator + 1));
      } else if (next != nullptr) {
        this->options_[canonical].emplace_back(next);
        index += 1;
      } else {
        throw OptionsMissingOptionValueError(name);
      }

      // Parse short options
    } else if (token.size() >= 2 && token[0] == '-' && token[1] != '-') {
      for (std::size_t flag = 1; flag < token.size(); flag++) {
        const auto name{token.substr(flag, 1)};
        const auto &canonical{find_canonical_name(this->aliases_, name)};
        const auto is_flag{this->flags_.contains(canonical)};

        if (is_flag) {
          this->options_[canonical].emplace_back();
        } else if (flag + 1 < token.size()) {
          this->options_[canonical].push_back(token.substr(flag + 1));
          break;
        } else if (next != nullptr) {
          this->options_[canonical].emplace_back(next);
          index += 1;
          break;
        } else {
          throw OptionsMissingOptionValueError(name);
        }
      }

      // Otherwise parse as positional
    } else {
      this->options_[POSITIONAL_ARGUMENT_NAME].emplace_back(token);
    }
  }
}

} // namespace sourcemeta::core
