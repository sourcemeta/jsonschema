#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/unicode.h>

#include "entity_hash.h"

#include <algorithm> // std::max, std::ranges::find, std::ranges::stable_sort
#include <cstddef>   // std::size_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <ios>         // std::hex, std::uppercase, std::dec
#include <iostream>    // std::cerr
#include <numeric>     // std::iota
#include <optional>    // std::optional, std::nullopt
#include <ostream>     // std::ostream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view, std::string_view_literals
#include <utility>     // std::move
#include <vector>      // std::vector

namespace {

using namespace std::string_view_literals;

constexpr auto HASH_CODEPOINTS{
    sourcemeta::core::JSON::Object::hash("codepoints"sv)};

constexpr std::size_t CHARACTERS_CAPACITY{8};
constexpr std::uint64_t MAXIMUM_SEED_ATTEMPTS{1000};
constexpr std::uint32_t MAXIMUM_DISPLACEMENT{0xFFFF};
// Every bucket holds this many names on average
constexpr std::uint32_t BUCKET_SIZE{3};

struct Entity {
  std::string name;
  std::string characters;
};

struct PerfectHash {
  std::uint64_t seed;
  std::uint32_t buckets;
  std::vector<std::uint16_t> displacements;
  std::vector<std::uint32_t> slots;
};

auto parse_entities(const std::filesystem::path &input_path)
    -> std::vector<Entity> {
  const auto document{sourcemeta::core::read_json(input_path)};
  if (!document.is_object()) {
    throw std::runtime_error{"The entities must be an object"};
  }

  std::vector<Entity> result;
  result.reserve(document.object_size());
  for (const auto &entry : document.as_object()) {
    const auto &key{entry.first};
    const auto *codepoints{
        entry.second.is_object()
            ? entry.second.try_at("codepoints"sv, HASH_CODEPOINTS)
            : nullptr};
    if (key.size() < 2 || key.front() != '&' || codepoints == nullptr ||
        !codepoints->is_array()) {
      throw std::runtime_error{"Invalid entity definition"};
    }

    Entity entity{.name = std::string{key.data() + 1, key.size() - 1},
                  .characters = {}};
    for (const auto &codepoint : codepoints->as_array()) {
      if (!codepoint.is_integer() || codepoint.to_integer() < 0 ||
          codepoint.to_integer() > 0x10FFFF ||
          !sourcemeta::core::is_valid_codepoint(
              static_cast<char32_t>(codepoint.to_integer()))) {
        throw std::runtime_error{
            std::string{"Invalid codepoint in entity: "}.append(entity.name)};
      }

      sourcemeta::core::codepoint_to_utf8(
          static_cast<char32_t>(codepoint.to_integer()), entity.characters);
    }

    if (entity.characters.empty() ||
        entity.characters.size() > CHARACTERS_CAPACITY) {
      throw std::runtime_error{
          std::string{"Invalid characters in entity: "}.append(entity.name)};
    }

    result.push_back(std::move(entity));
  }

  if (result.empty()) {
    throw std::runtime_error{"No entities found"};
  }

  return result;
}

// A displacement that sends every name of a bucket to a different free slot
auto find_displacement(const std::vector<std::uint64_t> &hashes,
                       const std::vector<std::size_t> &members,
                       const std::vector<bool> &occupied,
                       const std::uint32_t slot_count,
                       std::vector<std::uint32_t> &candidates)
    -> std::optional<std::uint16_t> {
  for (std::uint32_t displacement{0}; displacement <= MAXIMUM_DISPLACEMENT;
       ++displacement) {
    candidates.clear();
    bool valid{true};
    for (const auto member : members) {
      const auto slot{sourcemeta::core::html_entity_slot(
          hashes[member], displacement, slot_count)};
      if (occupied[slot] ||
          std::ranges::find(candidates, slot) != candidates.end()) {
        valid = false;
        break;
      }

      candidates.push_back(slot);
    }

    if (valid) {
      return static_cast<std::uint16_t>(displacement);
    }
  }

  return std::nullopt;
}

// A minimal perfect hash of the names, placing the largest buckets first while
// most slots are still free
auto build_perfect_hash(const std::vector<Entity> &entries) -> PerfectHash {
  const auto slot_count{static_cast<std::uint32_t>(entries.size())};
  const auto bucket_count{(slot_count + BUCKET_SIZE - 1) / BUCKET_SIZE};
  std::vector<std::uint64_t> hashes(entries.size(), 0);
  std::vector<std::uint32_t> candidates;
  for (std::uint64_t seed{0}; seed < MAXIMUM_SEED_ATTEMPTS; ++seed) {
    std::vector<std::vector<std::size_t>> members(bucket_count);
    for (std::size_t index{0}; index < entries.size(); ++index) {
      hashes[index] =
          sourcemeta::core::html_entity_hash(entries[index].name, seed);
      members[sourcemeta::core::html_entity_bucket(hashes[index], bucket_count)]
          .push_back(index);
    }

    std::vector<std::uint32_t> order(bucket_count, 0);
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(
        order, [&members](const std::uint32_t left, const std::uint32_t right) {
          return members[left].size() > members[right].size();
        });

    PerfectHash result{.seed = seed,
                       .buckets = bucket_count,
                       .displacements =
                           std::vector<std::uint16_t>(bucket_count, 0),
                       .slots = std::vector<std::uint32_t>(entries.size(), 0)};
    std::vector<bool> occupied(slot_count, false);
    bool complete{true};
    for (const auto bucket : order) {
      if (members[bucket].empty()) {
        break;
      }

      const auto displacement{find_displacement(
          hashes, members[bucket], occupied, slot_count, candidates)};
      if (!displacement.has_value()) {
        complete = false;
        break;
      }

      result.displacements[bucket] = displacement.value();
      for (const auto member : members[bucket]) {
        const auto slot{sourcemeta::core::html_entity_slot(
            hashes[member], displacement.value(), slot_count)};
        occupied[slot] = true;
        result.slots[member] = slot;
      }
    }

    if (complete) {
      return result;
    }
  }

  throw std::runtime_error{"Could not build a perfect hash of the entities"};
}

auto emit_byte(std::ostream &stream, const char character) -> void {
  stream << "'\\x" << std::hex << std::uppercase
         << static_cast<unsigned int>(static_cast<unsigned char>(character))
         << std::dec << "'";
}

auto emit_entities(std::ostream &stream, const std::vector<Entity> &entries,
                   const PerfectHash &table) -> void {
  std::vector<std::size_t> by_slot(entries.size(), 0);
  std::size_t maximum_name_length{0};
  for (std::size_t index{0}; index < entries.size(); ++index) {
    by_slot[table.slots[index]] = index;
    maximum_name_length =
        std::max(maximum_name_length, entries[index].name.size());
  }

  stream << "#include <cstddef>\n";
  stream << "#include <cstdint>\n\n";
  stream << "namespace {\n\n";
  stream << "constexpr std::uint64_t HTML_ENTITY_SEED{" << table.seed << "};\n";
  stream << "constexpr std::uint32_t HTML_ENTITY_BUCKETS{" << table.buckets
         << "};\n";
  stream << "constexpr std::uint32_t HTML_ENTITY_COUNT{" << entries.size()
         << "};\n";
  stream << "constexpr std::size_t HTML_ENTITY_MAXIMUM_NAME_LENGTH{"
         << maximum_name_length << "};\n\n";

  stream << "constexpr std::uint16_t HTML_ENTITY_DISPLACEMENTS["
         << table.buckets << "] = {\n";
  for (std::size_t index{0}; index < table.displacements.size(); ++index) {
    stream << (index % 16 == 0 ? "    " : " ") << table.displacements[index]
           << ",";
    if (index % 16 == 15 || index + 1 == table.displacements.size()) {
      stream << "\n";
    }
  }
  stream << "};\n\n";

  stream << "constexpr char HTML_ENTITY_NAMES[] =\n";
  std::size_t offset{0};
  std::vector<std::size_t> offsets(entries.size(), 0);
  for (const auto index : by_slot) {
    offsets[index] = offset;
    stream << "    \"" << entries[index].name << "\"\n";
    offset += entries[index].name.size();
  }
  stream << "    ;\n\n";
  if (offset > 0xFFFF) {
    throw std::runtime_error{"Entity names do not fit in the offsets"};
  }

  stream << "struct HTMLEntityEntry {\n"
         << "  std::uint16_t name_offset;\n"
         << "  std::uint8_t name_length;\n"
         << "  std::uint8_t characters_length;\n"
         << "  char characters[" << CHARACTERS_CAPACITY << "];\n"
         << "};\n\n";
  stream << "constexpr HTMLEntityEntry HTML_ENTITIES[" << entries.size()
         << "] = {\n";
  for (const auto index : by_slot) {
    const auto &entry{entries[index]};
    stream << "    {" << offsets[index] << ", " << entry.name.size() << ", "
           << entry.characters.size() << ", {";
    for (std::size_t position{0}; position < entry.characters.size();
         ++position) {
      if (position > 0) {
        stream << ", ";
      }

      emit_byte(stream, entry.characters[position]);
    }
    stream << "}},\n";
  }
  stream << "};\n\n";
  stream << "} // namespace\n";
}

} // namespace

auto main(const int argc, const char *const argv[]) -> int {
  try {
    sourcemeta::core::Options app;
    app.parse(argc, argv);
    const auto &positional{app.positional()};
    if (positional.size() != 2) {
      std::cerr << "Usage: " << (argc > 0 ? argv[0] : "codegen")
                << " <output.h> <entities.json>\n";
      return EXIT_FAILURE;
    }

    const std::filesystem::path output_path{positional.at(0)};
    const auto entries{parse_entities(positional.at(1))};
    const auto table{build_perfect_hash(entries)};
    sourcemeta::core::write_file(output_path, [&](std::ostream &stream) {
      emit_entities(stream, entries, table);
    });
  } catch (const std::exception &error) {
    std::cerr << "codegen: " << error.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
