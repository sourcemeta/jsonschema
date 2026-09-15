#include <sourcemeta/core/html_entity.h>

#include "entity_hash.h"
#include "html_entities.h"

#include <cstring>     // std::memcmp
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto html_entity(const std::string_view name) noexcept -> std::string_view {
  if (name.empty() || name.size() > HTML_ENTITY_MAXIMUM_NAME_LENGTH) {
    return {};
  }

  const auto hash{html_entity_hash(name, HTML_ENTITY_SEED)};
  const auto displacement{
      HTML_ENTITY_DISPLACEMENTS[html_entity_bucket(hash, HTML_ENTITY_BUCKETS)]};
  const auto &entry{
      HTML_ENTITIES[html_entity_slot(hash, displacement, HTML_ENTITY_COUNT)]};
  if (entry.name_length != name.size() ||
      std::memcmp(HTML_ENTITY_NAMES + entry.name_offset, name.data(),
                  name.size()) != 0) {
    return {};
  }

  return {entry.characters, entry.characters_length};
}

} // namespace sourcemeta::core
