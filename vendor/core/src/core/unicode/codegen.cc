#include <sourcemeta/core/unicode_ucd.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/text.h>

#include <algorithm> // std::sort
#include <array>     // std::array
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>     // std::exception
#include <filesystem>    // std::filesystem::path
#include <iomanip>       // std::hex, std::uppercase, std::dec
#include <ios>           // std::ios
#include <iostream>      // std::cerr
#include <map>           // std::map
#include <optional>      // std::optional
#include <ostream>       // std::ostream
#include <ranges>        // std::views::transform
#include <span>          // std::span
#include <stdexcept>     // std::runtime_error
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

namespace {

constexpr std::size_t TOTAL_CODEPOINTS{0x110000};
constexpr std::size_t PAGE_SHIFT{10};
constexpr std::size_t PAGE_SIZE{1 << PAGE_SHIFT};
constexpr std::size_t NUM_PAGES{TOTAL_CODEPOINTS / PAGE_SIZE};
constexpr std::size_t DECOMPOSITION_OFFSET_BITS{14};
constexpr std::size_t DECOMPOSITION_OFFSET_MASK{
    (1U << DECOMPOSITION_OFFSET_BITS) - 1U};

constexpr auto JOINING_TYPE_ORDER{std::to_array<std::string_view>({
#define SOURCEMETA_CORE_UCD_ALIAS_ENTRY(name, alias) alias,
    SOURCEMETA_CORE_JOINING_TYPE_LIST(SOURCEMETA_CORE_UCD_ALIAS_ENTRY)
#undef SOURCEMETA_CORE_UCD_ALIAS_ENTRY
})};

constexpr auto BIDI_CLASS_ORDER{std::to_array<std::string_view>({
#define SOURCEMETA_CORE_UCD_ALIAS_ENTRY(name, alias) alias,
    SOURCEMETA_CORE_BIDI_CLASS_LIST(SOURCEMETA_CORE_UCD_ALIAS_ENTRY)
#undef SOURCEMETA_CORE_UCD_ALIAS_ENTRY
})};

constexpr auto NFC_QUICK_CHECK_ORDER{std::to_array<std::string_view>({
#define SOURCEMETA_CORE_UCD_ALIAS_ENTRY(name, alias) alias,
    SOURCEMETA_CORE_NFC_QUICK_CHECK_LIST(SOURCEMETA_CORE_UCD_ALIAS_ENTRY)
#undef SOURCEMETA_CORE_UCD_ALIAS_ENTRY
})};

constexpr auto UNICODE_SCRIPT_ORDER{std::to_array<std::string_view>({
#define SOURCEMETA_CORE_UCD_ALIAS_ENTRY(name, alias) alias,
    SOURCEMETA_CORE_UNICODE_SCRIPT_LIST(SOURCEMETA_CORE_UCD_ALIAS_ENTRY)
#undef SOURCEMETA_CORE_UCD_ALIAS_ENTRY
})};

using ValueMap = std::map<std::string, std::uint8_t, std::less<>>;

struct PropertyEntry {
  std::uint32_t first;
  std::uint32_t last;
  std::uint8_t value;
};

struct TwoStageTable {
  std::vector<std::uint16_t> stage1;
  std::vector<std::uint8_t> stage2;
};

struct DecompositionTable {
  std::vector<char32_t> blob;
  std::vector<std::uint16_t> stage1;
  std::vector<std::uint16_t> stage2;
};

struct CanonicalCompositionTriple {
  std::uint32_t starter;
  std::uint32_t combining;
  std::uint32_t composed;
};

auto parse_hex_codepoint(const std::string_view token) -> std::uint32_t {
  const auto parsed{sourcemeta::core::to_uint32_t(token, 16)};
  if (!parsed.has_value() || parsed.value() > 0x10FFFF) {
    throw std::runtime_error{std::string{"Invalid codepoint: "}.append(token)};
  }
  return parsed.value();
}

auto parse_property_file(const std::filesystem::path &input_path,
                         const ValueMap &value_map,
                         const std::optional<std::string_view> property_filter)
    -> std::vector<PropertyEntry> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::vector<PropertyEntry> missing;
  std::vector<PropertyEntry> data;
  constexpr std::string_view missing_prefix{"@missing:"};

  const auto parse_payload{
      [&](const std::string_view payload) -> std::optional<PropertyEntry> {
        const auto trimmed{
            sourcemeta::core::trim(sourcemeta::core::take_until(payload, '#'))};
        const auto first_split{sourcemeta::core::split_once(trimmed, ';')};
        if (!first_split.has_value()) {
          throw std::runtime_error{
              std::string{"Unparseable line: "}.append(payload)};
        }
        const auto range_part{sourcemeta::core::trim(first_split->first)};
        const auto after_first{sourcemeta::core::trim(first_split->second)};

        std::string_view value_token;
        if (property_filter.has_value()) {
          const auto second_split{
              sourcemeta::core::split_once(after_first, ';')};
          std::string_view property_token;
          std::string_view tail;
          if (second_split.has_value()) {
            property_token = sourcemeta::core::trim(second_split->first);
            tail = sourcemeta::core::trim(second_split->second);
          } else {
            property_token = after_first;
            tail = {};
          }
          if (property_token != property_filter.value()) {
            return std::nullopt;
          }
          value_token = tail;
        } else {
          value_token = after_first;
        }

        const auto range_split{
            sourcemeta::core::split_once(range_part, std::string_view{".."})};
        const auto first{parse_hex_codepoint(
            range_split.has_value() ? range_split->first : range_part)};
        const auto last{range_split.has_value()
                            ? parse_hex_codepoint(range_split->second)
                            : first};

        const auto value_it{value_map.find(value_token)};
        if (value_it == value_map.end()) {
          throw std::runtime_error{
              std::string{"Unknown property value: "}.append(value_token)};
        }
        return PropertyEntry{first, last, value_it->second};
      }};

  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{sourcemeta::core::trim(raw_line)};
    if (line.empty()) {
      return;
    }
    if (line.front() == '#') {
      const auto comment_body{sourcemeta::core::trim(line.substr(1))};
      if (comment_body.size() < missing_prefix.size() ||
          comment_body.substr(0, missing_prefix.size()) != missing_prefix) {
        return;
      }
      const auto entry{
          parse_payload(comment_body.substr(missing_prefix.size()))};
      if (entry.has_value()) {
        missing.push_back(entry.value());
      }
      return;
    }
    const auto entry{parse_payload(line)};
    if (entry.has_value()) {
      data.push_back(entry.value());
    }
  });

  std::vector<PropertyEntry> result;
  result.reserve(missing.size() + data.size());
  result.insert(result.end(), missing.begin(), missing.end());
  result.insert(result.end(), data.begin(), data.end());
  return result;
}

auto parse_alias_rows(const std::filesystem::path &aliases_path,
                      const std::string_view property_short)
    -> std::vector<std::vector<std::string>> {
  auto stream{sourcemeta::core::read_file(aliases_path)};
  std::vector<std::vector<std::string>> rows;
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{
        sourcemeta::core::trim(sourcemeta::core::take_until(raw_line, '#'))};
    if (line.empty()) {
      return;
    }
    std::vector<std::string> row;
    bool matched{false};
    std::size_t field_index{0};
    sourcemeta::core::split(line, ';', [&](const std::string_view field) {
      const auto trimmed{sourcemeta::core::trim(field)};
      if (field_index == 0) {
        matched = (trimmed == property_short);
      } else if (matched && !trimmed.empty()) {
        row.emplace_back(trimmed);
      }
      field_index += 1;
    });
    if (matched) {
      rows.push_back(std::move(row));
    }
  });
  return rows;
}

auto build_combining_mark_value_map(const std::filesystem::path &aliases_path)
    -> ValueMap {
  static constexpr std::array<std::string_view, 4> combining{
      {"M", "Mn", "Mc", "Me"}};
  ValueMap result;
  for (const auto &row : parse_alias_rows(aliases_path, "gc")) {
    std::uint8_t value{0};
    for (const auto &field : row) {
      for (const auto &candidate : combining) {
        if (field == candidate) {
          value = 1;
          break;
        }
      }
      if (value == 1) {
        break;
      }
    }
    for (const auto &field : row) {
      result[field] = value;
    }
  }
  return result;
}

auto build_value_map(const std::filesystem::path &aliases_path,
                     const std::string_view property_short,
                     const std::span<const std::string_view> canonical_order)
    -> ValueMap {
  std::unordered_map<std::string_view, std::uint8_t> canonical_to_int;
  canonical_to_int.reserve(canonical_order.size());
  for (std::size_t index{0}; index < canonical_order.size(); index += 1) {
    canonical_to_int.emplace(canonical_order[index],
                             static_cast<std::uint8_t>(index));
  }
  ValueMap result;
  std::vector<std::vector<std::string>> unmatched;
  for (const auto &row : parse_alias_rows(aliases_path, property_short)) {
    std::optional<std::uint8_t> value;
    for (const auto &field : row) {
      const auto found{canonical_to_int.find(field)};
      if (found != canonical_to_int.end()) {
        value = found->second;
        break;
      }
    }
    if (!value.has_value()) {
      unmatched.push_back(row);
      continue;
    }
    for (const auto &field : row) {
      result[field] = value.value();
    }
  }
  if (!unmatched.empty()) {
    throw std::runtime_error{
        std::string{"Property has values not in canonical order: "}.append(
            property_short)};
  }
  return result;
}

auto build_integer_value_map(const std::filesystem::path &aliases_path,
                             const std::string_view property_short)
    -> ValueMap {
  ValueMap result;
  for (const auto &row : parse_alias_rows(aliases_path, property_short)) {
    if (row.empty()) {
      continue;
    }
    const auto parsed{sourcemeta::core::to_uint32_t(row.front())};
    if (!parsed.has_value() || parsed.value() > 0xFFU) {
      throw std::runtime_error{
          std::string{"Invalid integer property value: "}.append(row.front())};
    }
    const auto value{static_cast<std::uint8_t>(parsed.value())};
    for (const auto &field : row) {
      result[field] = value;
    }
  }
  return result;
}

struct UnicodeData {
  std::map<std::uint32_t, std::vector<std::uint32_t>> decompositions;
  std::unordered_map<std::uint32_t, std::uint8_t> ccc;
};

auto parse_unicode_data(const std::filesystem::path &input_path)
    -> UnicodeData {
  auto stream{sourcemeta::core::read_file(input_path)};
  UnicodeData result;
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{sourcemeta::core::trim(raw_line)};
    if (line.empty() || line.front() == '#') {
      return;
    }
    std::array<std::string_view, 6> fields{};
    std::size_t field_count{0};
    sourcemeta::core::split(line, ';', [&](const std::string_view field) {
      if (field_count < fields.size()) {
        fields[field_count] = field;
      }
      field_count += 1;
    });
    if (field_count < fields.size()) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: too few fields in line: "}.append(
              line)};
    }
    const auto codepoint{
        parse_hex_codepoint(sourcemeta::core::trim(fields[0]))};
    const auto ccc_token{sourcemeta::core::trim(fields[3])};
    const auto ccc_value{sourcemeta::core::to_uint32_t(ccc_token)};
    if (!ccc_value.has_value() || ccc_value.value() > 0xFFU) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: invalid CCC: "}.append(ccc_token)};
    }
    if (ccc_value.value() != 0) {
      result.ccc[codepoint] = static_cast<std::uint8_t>(ccc_value.value());
    }
    const auto decomp_field{sourcemeta::core::trim(fields[5])};
    if (decomp_field.empty() || decomp_field.front() == '<') {
      return;
    }
    std::vector<std::uint32_t> decomposition;
    std::string_view rest{decomp_field};
    while (!rest.empty()) {
      const auto token_end{rest.find(' ')};
      const auto token{token_end == std::string_view::npos
                           ? rest
                           : rest.substr(0, token_end)};
      decomposition.push_back(parse_hex_codepoint(token));
      if (token_end == std::string_view::npos) {
        break;
      }
      rest.remove_prefix(token_end + 1);
      while (!rest.empty() && rest.front() == ' ') {
        rest.remove_prefix(1);
      }
    }
    if (decomposition.size() > 2) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: canonical decomposition has more "
                      "than 2 codepoints"}};
    }
    result.decompositions[codepoint] = std::move(decomposition);
  });
  return result;
}

auto parse_full_composition_exclusions(const std::filesystem::path &input_path)
    -> std::unordered_set<std::uint32_t> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::unordered_set<std::uint32_t> result;
  constexpr std::string_view target{"Full_Composition_Exclusion"};
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{sourcemeta::core::trim(raw_line)};
    if (line.empty() || line.front() == '#') {
      return;
    }
    const auto trimmed{
        sourcemeta::core::trim(sourcemeta::core::take_until(line, '#'))};
    const auto first_split{sourcemeta::core::split_once(trimmed, ';')};
    if (!first_split.has_value()) {
      throw std::runtime_error{
          std::string{"DerivedNormalizationProps.txt: unparseable line: "}
              .append(line)};
    }
    const auto range_part{sourcemeta::core::trim(first_split->first)};
    const auto after_first{sourcemeta::core::trim(first_split->second)};
    const auto second_split{sourcemeta::core::split_once(after_first, ';')};
    const auto property_token{second_split.has_value()
                                  ? sourcemeta::core::trim(second_split->first)
                                  : after_first};
    if (property_token != target) {
      return;
    }
    const auto range_split{
        sourcemeta::core::split_once(range_part, std::string_view{".."})};
    const auto first{parse_hex_codepoint(
        range_split.has_value() ? range_split->first : range_part)};
    const auto last{range_split.has_value()
                        ? parse_hex_codepoint(range_split->second)
                        : first};
    for (std::uint32_t codepoint{first}; codepoint <= last; codepoint += 1) {
      result.insert(codepoint);
    }
  });
  return result;
}

auto parse_explicit_composition_exclusions(
    const std::filesystem::path &input_path)
    -> std::unordered_set<std::uint32_t> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::unordered_set<std::uint32_t> result;
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{sourcemeta::core::trim(raw_line)};
    if (line.empty() || line.front() == '#') {
      return;
    }
    const auto trimmed{
        sourcemeta::core::trim(sourcemeta::core::take_until(line, '#'))};
    if (trimmed.empty()) {
      return;
    }
    result.insert(parse_hex_codepoint(trimmed));
  });
  return result;
}

auto build_canonical_compositions(
    const std::map<std::uint32_t, std::vector<std::uint32_t>> &decompositions,
    const std::unordered_map<std::uint32_t, std::uint8_t> &ccc,
    const std::unordered_set<std::uint32_t> &full_exclusions,
    const std::unordered_set<std::uint32_t> &explicit_exclusions)
    -> std::vector<CanonicalCompositionTriple> {
  for (const auto codepoint : explicit_exclusions) {
    if (!full_exclusions.contains(codepoint)) {
      throw std::runtime_error{
          std::string{"CompositionExclusions.txt has entries missing from "
                      "Full_Composition_Exclusion"}};
    }
  }

  const auto ccc_of{[&](const std::uint32_t codepoint) -> std::uint8_t {
    const auto found{ccc.find(codepoint)};
    return found == ccc.end() ? std::uint8_t{0} : found->second;
  }};

  std::vector<CanonicalCompositionTriple> triples;
  for (const auto &[composed, decomposition] : decompositions) {
    if (decomposition.size() != 2) {
      continue;
    }
    if (ccc_of(composed) != 0) {
      continue;
    }
    if (ccc_of(decomposition[0]) != 0) {
      continue;
    }
    if (full_exclusions.contains(composed)) {
      continue;
    }
    triples.push_back({decomposition[0], decomposition[1], composed});
  }
  std::sort(triples.begin(), triples.end(),
            [](const CanonicalCompositionTriple &left,
               const CanonicalCompositionTriple &right) {
              if (left.starter != right.starter) {
                return left.starter < right.starter;
              }
              if (left.combining != right.combining) {
                return left.combining < right.combining;
              }
              return left.composed < right.composed;
            });
  return triples;
}

auto build_canonical_decomposition_pages(
    const std::map<std::uint32_t, std::vector<std::uint32_t>> &decompositions)
    -> DecompositionTable {
  std::vector<char32_t> blob;
  std::vector<std::uint16_t> packed(TOTAL_CODEPOINTS, 0);
  for (const auto &[codepoint, decomposition] : decompositions) {
    const auto offset{blob.size()};
    if (offset > DECOMPOSITION_OFFSET_MASK) {
      throw std::runtime_error{
          std::string{"canonical decomposition blob exceeds offset cap"}};
    }
    for (const auto value : decomposition) {
      blob.push_back(static_cast<char32_t>(value));
    }
    packed[codepoint] = static_cast<std::uint16_t>(
        (decomposition.size() << DECOMPOSITION_OFFSET_BITS) | offset);
  }

  std::unordered_map<std::string, std::uint16_t> page_to_id;
  DecompositionTable table;
  table.blob = std::move(blob);
  table.stage1.reserve(NUM_PAGES);
  for (std::size_t page_index{0}; page_index < NUM_PAGES; page_index += 1) {
    const auto page_start{page_index * PAGE_SIZE};
    const std::string page_key{
        reinterpret_cast<const char *>(packed.data() + page_start),
        PAGE_SIZE * sizeof(std::uint16_t)};
    const auto existing{page_to_id.find(page_key)};
    if (existing != page_to_id.end()) {
      table.stage1.push_back(existing->second);
      continue;
    }
    const auto new_id{
        static_cast<std::uint16_t>(table.stage2.size() / PAGE_SIZE)};
    page_to_id.emplace(page_key, new_id);
    table.stage2.insert(
        table.stage2.end(),
        packed.begin() + static_cast<std::ptrdiff_t>(page_start),
        packed.begin() + static_cast<std::ptrdiff_t>(page_start + PAGE_SIZE));
    table.stage1.push_back(new_id);
  }
  return table;
}

auto build_pages(const std::vector<PropertyEntry> &entries) -> TwoStageTable {
  std::vector<std::uint8_t> values(TOTAL_CODEPOINTS, 0);
  for (const auto &entry : entries) {
    for (std::uint32_t codepoint{entry.first}; codepoint <= entry.last;
         codepoint += 1) {
      values[codepoint] = entry.value;
    }
  }

  std::unordered_map<std::string, std::uint16_t> page_to_id;
  TwoStageTable table;
  table.stage1.reserve(NUM_PAGES);
  for (std::size_t page_index{0}; page_index < NUM_PAGES; page_index += 1) {
    const auto page_start{page_index * PAGE_SIZE};
    const std::string page_key{
        reinterpret_cast<const char *>(values.data() + page_start), PAGE_SIZE};
    const auto existing{page_to_id.find(page_key)};
    if (existing != page_to_id.end()) {
      table.stage1.push_back(existing->second);
      continue;
    }
    const auto new_id{
        static_cast<std::uint16_t>(table.stage2.size() / PAGE_SIZE)};
    page_to_id.emplace(page_key, new_id);
    table.stage2.insert(
        table.stage2.end(),
        values.begin() + static_cast<std::ptrdiff_t>(page_start),
        values.begin() + static_cast<std::ptrdiff_t>(page_start + PAGE_SIZE));
    table.stage1.push_back(new_id);
  }
  return table;
}

template <typename T>
auto emit_row_decimal(std::ostream &stream, const std::span<const T> items)
    -> void {
  constexpr std::size_t row_width{16};
  for (std::size_t offset{0}; offset < items.size(); offset += row_width) {
    stream << "    ";
    const auto upper{offset + row_width < items.size() ? offset + row_width
                                                       : items.size()};
    const auto row{items.subspan(offset, upper - offset)};
    const auto widened{row | std::views::transform([](const T value) {
                         return static_cast<std::uint64_t>(value);
                       })};
    sourcemeta::core::join_to(stream, widened, ", ");
    stream << ",\n";
  }
}

template <typename T>
auto emit_row_hex(std::ostream &stream, const std::span<const T> items,
                  const std::size_t row_width) -> void {
  for (std::size_t offset{0}; offset < items.size(); offset += row_width) {
    stream << "    ";
    const auto upper{offset + row_width < items.size() ? offset + row_width
                                                       : items.size()};
    for (std::size_t column{offset}; column < upper; column += 1) {
      if (column > offset) {
        stream << ", ";
      }
      stream << "0x" << std::hex << std::uppercase
             << static_cast<std::uint64_t>(items[column]) << std::dec;
    }
    stream << ",\n";
  }
}

auto emit_property(std::ostream &stream, const std::string_view prefix,
                   const TwoStageTable &table) -> void {
  stream << "constexpr std::uint16_t " << prefix << "_STAGE1["
         << table.stage1.size() << "] = {\n";
  emit_row_decimal<std::uint16_t>(stream, table.stage1);
  stream << "};\n\n";
  stream << "constexpr std::uint8_t " << prefix << "_STAGE2["
         << table.stage2.size() << "] = {\n";
  emit_row_decimal<std::uint8_t>(stream, table.stage2);
  stream << "};\n\n";
}

auto emit_canonical_decomposition(std::ostream &stream,
                                  const DecompositionTable &table) -> void {
  stream << "constexpr char32_t CANONICAL_DECOMPOSITION_BLOB["
         << table.blob.size() << "] = {\n";
  emit_row_hex<char32_t>(stream, table.blob, 8);
  stream << "};\n\n";
  stream << "constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE1["
         << table.stage1.size() << "] = {\n";
  emit_row_decimal<std::uint16_t>(stream, table.stage1);
  stream << "};\n\n";
  stream << "constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE2["
         << table.stage2.size() << "] = {\n";
  emit_row_decimal<std::uint16_t>(stream, table.stage2);
  stream << "};\n\n";
}

auto emit_canonical_composition(
    std::ostream &stream,
    const std::vector<CanonicalCompositionTriple> &triples) -> void {
  stream << "struct CanonicalCompositionEntry {\n";
  stream << "  char32_t starter;\n";
  stream << "  char32_t combining;\n";
  stream << "  char32_t composed;\n";
  stream << "};\n\n";
  stream << "constexpr CanonicalCompositionEntry CANONICAL_COMPOSITIONS["
         << triples.size() << "] = {\n";
  for (const auto &triple : triples) {
    stream << "    {0x" << std::hex << std::uppercase
           << static_cast<std::uint64_t>(triple.starter) << ", 0x"
           << static_cast<std::uint64_t>(triple.combining) << ", 0x"
           << static_cast<std::uint64_t>(triple.composed) << std::dec << "},\n";
  }
  stream << "};\n\n";
}

} // namespace

auto main(const int argc, const char *const argv[]) -> int {
  try {
    sourcemeta::core::Options app;
    app.parse(argc, argv);
    const auto &positional{app.positional()};
    if (positional.size() != 10) {
      std::cerr
          << "Usage: " << (argc > 0 ? argv[0] : "codegen")
          << " <output.h> <PropertyValueAliases.txt>"
             " <DerivedCombiningClass.txt> <DerivedJoiningType.txt>"
             " <DerivedBidiClass.txt> <Scripts.txt>"
             " <DerivedGeneralCategory.txt> <DerivedNormalizationProps.txt>"
             " <UnicodeData.txt> <CompositionExclusions.txt>\n";
      return EXIT_FAILURE;
    }

    const std::filesystem::path output_path{positional.at(0)};
    const std::filesystem::path aliases_path{positional.at(1)};
    const std::filesystem::path combining_class_path{positional.at(2)};
    const std::filesystem::path joining_type_path{positional.at(3)};
    const std::filesystem::path bidi_class_path{positional.at(4)};
    const std::filesystem::path scripts_path{positional.at(5)};
    const std::filesystem::path general_category_path{positional.at(6)};
    const std::filesystem::path normalization_props_path{positional.at(7)};
    const std::filesystem::path unicode_data_path{positional.at(8)};
    const std::filesystem::path composition_exclusions_path{positional.at(9)};

    const auto unicode_data{parse_unicode_data(unicode_data_path)};
    const auto full_exclusions{
        parse_full_composition_exclusions(normalization_props_path)};
    const auto explicit_exclusions{
        parse_explicit_composition_exclusions(composition_exclusions_path)};

    sourcemeta::core::write_file(output_path, [&](std::ostream &stream) {
      stream << "#include <cstddef>\n";
      stream << "#include <cstdint>\n\n";
      stream << "namespace {\n\n";

      struct PropertySpec {
        std::string_view prefix;
        const std::filesystem::path &input_path;
        std::optional<std::string_view> property_filter;
        ValueMap value_map;
      };

      const auto combining_class_map{
          build_integer_value_map(aliases_path, "ccc")};
      const auto joining_type_map{
          build_value_map(aliases_path, "jt", JOINING_TYPE_ORDER)};
      const auto bidi_class_map{
          build_value_map(aliases_path, "bc", BIDI_CLASS_ORDER)};
      const auto script_map{
          build_value_map(aliases_path, "sc", UNICODE_SCRIPT_ORDER)};
      const auto combining_mark_map{
          build_combining_mark_value_map(aliases_path)};
      const auto nfc_quick_check_map{
          build_value_map(aliases_path, "NFC_QC", NFC_QUICK_CHECK_ORDER)};

      const std::array<PropertySpec, 6> properties{
          {{"COMBINING_CLASS", combining_class_path, std::nullopt,
            combining_class_map},
           {"JOINING_TYPE", joining_type_path, std::nullopt, joining_type_map},
           {"BIDI_CLASS", bidi_class_path, std::nullopt, bidi_class_map},
           {"UNICODE_SCRIPT", scripts_path, std::nullopt, script_map},
           {"IS_COMBINING_MARK", general_category_path, std::nullopt,
            combining_mark_map},
           {"NFC_QUICK_CHECK", normalization_props_path,
            std::optional<std::string_view>{"NFC_QC"}, nfc_quick_check_map}}};

      for (const auto &spec : properties) {
        const auto entries{parse_property_file(spec.input_path, spec.value_map,
                                               spec.property_filter)};
        const auto table{build_pages(entries)};
        emit_property(stream, spec.prefix, table);
      }

      const auto decomposition_table{
          build_canonical_decomposition_pages(unicode_data.decompositions)};
      emit_canonical_decomposition(stream, decomposition_table);

      const auto triples{build_canonical_compositions(
          unicode_data.decompositions, unicode_data.ccc, full_exclusions,
          explicit_exclusions)};
      emit_canonical_composition(stream, triples);

      stream << "} // namespace\n";
    });
  } catch (const std::exception &error) {
    std::cerr << "codegen: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
