#include <sourcemeta/core/options.h>

#include <cassert> // assert
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
    -> const typename T::mapped_type & {
  const auto iterator{aliases.find(alias)};
  if (iterator == aliases.cend()) {
    throw sourcemeta::core::OptionsUnknownOptionError(std::string{alias});
  } else {
    return iterator->second;
  }
}

} // namespace

namespace sourcemeta::core {

const std::vector<std::string_view> Options::EMPTY = {};

auto Options::option(std::string &&name,
                     std::initializer_list<std::string> aliases) -> void {
  assert(!name.empty());
  const std::string_view view{emplace_back_unique(this->storage, name)};
  this->aliases_.try_emplace(view, view);
  for (const auto &alias : aliases) {
    assert(!alias.empty());
    const std::string_view alias_view{
        emplace_back_unique(this->storage, alias)};
    this->aliases_.try_emplace(alias_view, view);
  }
}

auto Options::flag(std::string &&name,
                   std::initializer_list<std::string> aliases) -> void {
  assert(!name.empty());
  const std::string_view view{emplace_back_unique(this->storage, name)};
  this->aliases_.try_emplace(view, view);
  for (const auto &alias : aliases) {
    assert(!alias.empty());
    const std::string_view alias_view{
        emplace_back_unique(this->storage, alias)};
    this->aliases_.try_emplace(alias_view, view);
  }

  this->flags.emplace(view);
}

auto Options::at(std::string_view name) const
    -> const std::vector<std::string_view> & {
  assert(!name.empty());
  const auto iterator{this->options_.find(name)};
  return iterator == this->options_.cend() ? Options::EMPTY : iterator->second;
}

auto Options::contains(std::string_view name) const -> bool {
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
  for (auto index = static_cast<int>(options.skip + 1); index < argc; index++) {
    const std::string_view token{argv[index]};

    if (end_of_options) {
      this->options_[POSITIONAL_ARGUMENT_NAME].emplace_back(token);
      continue;
    } else if (token == "--") {
      end_of_options = true;
      continue;
    }

    const auto *const next{(index + 1) < argc ? argv[index + 1] : nullptr};

    // Parse long options
    if (token.size() >= 3 && token[0] == '-' && token[1] == '-') {
      const auto eq{token.find('=')};
      const auto name{(eq == std::string_view::npos) ? token.substr(2)
                                                     : token.substr(2, eq - 2)};
      const auto &canonical{find_canonical_name(this->aliases_, name)};
      const auto is_flag{this->flags.contains(canonical)};

      if (is_flag) {
        if (eq == std::string_view::npos) {
          this->options_[canonical].push_back(token.substr(2));
        } else {
          throw OptionsUnexpectedValueFlagError(std::string{name});
        }
      } else if (eq != std::string_view::npos) {
        this->options_[canonical].push_back(token.substr(eq + 1));
      } else if (next) {
        this->options_[canonical].emplace_back(next);
        index += 1;
      } else {
        throw OptionsMissingOptionValueError(std::string{name});
      }

      // Parse short options
    } else if (token.size() >= 2 && token[0] == '-' && token[1] != '-') {
      for (std::size_t flag = 1; flag < token.size(); flag++) {
        const auto name{token.substr(flag, 1)};
        const auto &canonical{find_canonical_name(this->aliases_, name)};
        const auto is_flag{this->flags.contains(canonical)};

        if (is_flag) {
          this->options_[canonical].emplace_back();
        } else if (flag + 1 < token.size()) {
          this->options_[canonical].push_back(token.substr(flag + 1));
          break;
        } else if (next) {
          this->options_[canonical].emplace_back(next);
          index += 1;
          break;
        } else {
          throw OptionsMissingOptionValueError(std::string{name});
        }
      }

      // Otherwise parse as positional
    } else {
      this->options_[POSITIONAL_ARGUMENT_NAME].emplace_back(token);
    }
  }
}

} // namespace sourcemeta::core
