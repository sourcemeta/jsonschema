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
#include <exception>   // std::exception
#include <filesystem>  // std::filesystem::path
#include <ios>         // std::hex, std::uppercase, std::dec
#include <iostream>    // std::cerr
#include <map>         // std::map
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <ranges>      // std::views::transform
#include <regex>       // std::regex, std::regex_match, std::smatch, std::cmatch
#include <span>        // std::span
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace {

constexpr std::size_t TOTAL_CODEPOINTS{0x110000};
constexpr std::size_t TABLE_PAGE_SHIFT{10};
constexpr std::size_t TABLE_PAGE_SIZE{1 << TABLE_PAGE_SHIFT};
constexpr std::size_t NUM_PAGES{TOTAL_CODEPOINTS / TABLE_PAGE_SIZE};
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

struct UCDEntry {
  std::uint32_t first;
  std::uint32_t last;
  std::string trailing;
  bool is_missing;
};

auto parse_hex_codepoint(const std::string_view token) -> std::uint32_t {
  const auto parsed{sourcemeta::core::to_uint32_t(token, 16)};
  if (!parsed.has_value() || parsed.value() > 0x10FFFF) {
    throw std::runtime_error{std::string{"Invalid codepoint: "}.append(token)};
  }
  return parsed.value();
}

auto canonical_index(const std::span<const std::string_view> order,
                     const std::span<const std::string_view> aliases)
    -> std::uint8_t {
  for (const auto alias : aliases) {
    for (std::size_t index{0}; index < order.size(); index += 1) {
      if (alias == order[index]) {
        return static_cast<std::uint8_t>(index);
      }
    }
  }
  throw std::runtime_error{"Alias not in canonical order"};
}

template <typename Callback>
auto for_each_ucd_entry(std::istream &stream, Callback callback) -> void {
  static const std::regex line_re{
      R"(^([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(.*)$)"};
  static const std::regex missing_re{R"(^#\s*@missing:\s*(.+?)\s*$)"};

  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto trimmed{sourcemeta::core::trim(raw_line)};
    if (trimmed.empty()) {
      return;
    }
    std::string content;
    bool is_missing{false};
    if (trimmed.front() == '#') {
      std::cmatch match;
      if (!std::regex_match(trimmed.data(), trimmed.data() + trimmed.size(),
                            match, missing_re)) {
        return;
      }
      content = match[1].str();
      is_missing = true;
    } else {
      content = std::string{
          sourcemeta::core::trim(sourcemeta::core::take_until(trimmed, '#'))};
    }
    std::smatch match;
    if (!std::regex_match(content, match, line_re)) {
      throw std::runtime_error{
          std::string{"Unparseable UCD line: "}.append(content)};
    }
    UCDEntry entry;
    entry.first = parse_hex_codepoint(match[1].str());
    entry.last =
        match[2].matched ? parse_hex_codepoint(match[2].str()) : entry.first;
    entry.trailing = match[3].str();
    entry.is_missing = is_missing;
    callback(entry);
  });
}

auto parse_property_file(const std::filesystem::path &input_path,
                         const ValueMap &value_map,
                         const std::optional<std::string_view> property_filter)
    -> std::vector<PropertyEntry> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::vector<PropertyEntry> missing;
  std::vector<PropertyEntry> data;
  for_each_ucd_entry(stream, [&](const UCDEntry &entry) {
    std::string_view value_token;
    if (property_filter.has_value()) {
      const auto split{sourcemeta::core::split_once(entry.trailing, ';')};
      const auto property_token{split.has_value()
                                    ? sourcemeta::core::trim(split->first)
                                    : std::string_view{entry.trailing}};
      if (property_token != property_filter.value()) {
        return;
      }
      value_token = split.has_value() ? sourcemeta::core::trim(split->second)
                                      : std::string_view{};
    } else {
      value_token = entry.trailing;
    }
    const auto value_it{value_map.find(value_token)};
    if (value_it == value_map.end()) {
      throw std::runtime_error{
          std::string{"Unknown property value: "}.append(value_token)};
    }
    (entry.is_missing ? missing : data)
        .push_back({entry.first, entry.last, value_it->second});
  });
  std::vector<PropertyEntry> result;
  result.reserve(missing.size() + data.size());
  result.insert(result.end(), missing.begin(), missing.end());
  result.insert(result.end(), data.begin(), data.end());
  return result;
}

auto parse_full_composition_exclusions(const std::filesystem::path &input_path)
    -> std::unordered_set<std::uint32_t> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::unordered_set<std::uint32_t> result;
  for_each_ucd_entry(stream, [&](const UCDEntry &entry) {
    const auto split{sourcemeta::core::split_once(entry.trailing, ';')};
    const auto property{split.has_value() ? sourcemeta::core::trim(split->first)
                                          : std::string_view{entry.trailing}};
    if (property != "Full_Composition_Exclusion") {
      return;
    }
    for (std::uint32_t codepoint{entry.first}; codepoint <= entry.last;
         codepoint += 1) {
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
    const auto line{
        sourcemeta::core::trim(sourcemeta::core::take_until(raw_line, '#'))};
    if (line.empty()) {
      return;
    }
    result.insert(parse_hex_codepoint(line));
  });
  return result;
}

template <typename ValueFn>
auto build_alias_map(const std::filesystem::path &aliases_path,
                     const std::string_view property_short, ValueFn value_fn)
    -> ValueMap {
  ValueMap result;
  auto stream{sourcemeta::core::read_file(aliases_path)};
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{
        sourcemeta::core::trim(sourcemeta::core::take_until(raw_line, '#'))};
    if (line.empty()) {
      return;
    }
    std::vector<std::string_view> fields;
    sourcemeta::core::split(line, ';', [&](const std::string_view field) {
      const auto trimmed{sourcemeta::core::trim(field)};
      if (!trimmed.empty()) {
        fields.push_back(trimmed);
      }
    });
    if (fields.empty() || fields.front() != property_short) {
      return;
    }
    if (fields.size() < 2) {
      throw std::runtime_error{std::string{
          "PropertyValueAliases.txt: missing aliases for property: "}
                                   .append(property_short)};
    }
    const std::span<const std::string_view> aliases{fields.begin() + 1,
                                                    fields.end()};
    const auto value{value_fn(aliases)};
    for (const auto alias : aliases) {
      result[std::string{alias}] = value;
    }
  });
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
  for_each_ucd_entry(stream, [&](const UCDEntry &entry) {
    if (entry.first != entry.last) {
      throw std::runtime_error{
          "UnicodeData.txt: codepoint range not supported"};
    }
    std::array<std::string_view, 5> rest{};
    std::size_t count{0};
    sourcemeta::core::split(entry.trailing, ';',
                            [&](const std::string_view field) {
                              if (count < rest.size()) {
                                rest[count] = field;
                              }
                              count += 1;
                            });
    if (count < rest.size()) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: too few fields in line: "}.append(
              entry.trailing)};
    }
    const auto ccc_token{sourcemeta::core::trim(rest[2])};
    const auto ccc_value{sourcemeta::core::to_uint32_t(ccc_token)};
    if (!ccc_value.has_value() || ccc_value.value() > 0xFFU) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: invalid CCC: "}.append(ccc_token)};
    }
    if (ccc_value.value() != 0) {
      result.ccc[entry.first] = static_cast<std::uint8_t>(ccc_value.value());
    }
    const auto decomp_field{sourcemeta::core::trim(rest[4])};
    if (decomp_field.empty() || decomp_field.front() == '<') {
      return;
    }
    std::vector<std::uint32_t> decomposition;
    sourcemeta::core::split(
        decomp_field, ' ', [&](const std::string_view token) {
          const auto trimmed{sourcemeta::core::trim(token)};
          if (!trimmed.empty()) {
            decomposition.push_back(parse_hex_codepoint(trimmed));
          }
        });
    if (decomposition.size() > 2) {
      throw std::runtime_error{
          std::string{"UnicodeData.txt: canonical decomposition has more than "
                      "2 codepoints"}};
    }
    result.decompositions[entry.first] = std::move(decomposition);
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
    if (decomposition.size() != 2 || ccc_of(composed) != 0 ||
        ccc_of(decomposition[0]) != 0 || full_exclusions.contains(composed)) {
      continue;
    }
    triples.push_back({decomposition[0], decomposition[1], composed});
  }
  std::sort(triples.begin(), triples.end(),
            [](const CanonicalCompositionTriple &left,
               const CanonicalCompositionTriple &right) {
              return std::tie(left.starter, left.combining, left.composed) <
                     std::tie(right.starter, right.combining, right.composed);
            });
  return triples;
}

template <typename T>
auto build_page_table(const std::span<const T> values)
    -> std::pair<std::vector<std::uint16_t>, std::vector<T>> {
  std::unordered_map<std::string, std::uint16_t> page_to_id;
  std::vector<std::uint16_t> stage1;
  std::vector<T> stage2;
  stage1.reserve(NUM_PAGES);
  for (std::size_t page_index{0}; page_index < NUM_PAGES; page_index += 1) {
    const auto page_start{page_index * TABLE_PAGE_SIZE};
    const std::string page_key{
        reinterpret_cast<const char *>(values.data() + page_start),
        TABLE_PAGE_SIZE * sizeof(T)};
    const auto existing{page_to_id.find(page_key)};
    if (existing != page_to_id.end()) {
      stage1.push_back(existing->second);
      continue;
    }
    const auto new_id{
        static_cast<std::uint16_t>(stage2.size() / TABLE_PAGE_SIZE)};
    page_to_id.emplace(page_key, new_id);
    stage2.insert(stage2.end(),
                  values.begin() + static_cast<std::ptrdiff_t>(page_start),
                  values.begin() + static_cast<std::ptrdiff_t>(
                                       page_start + TABLE_PAGE_SIZE));
    stage1.push_back(new_id);
  }
  return {std::move(stage1), std::move(stage2)};
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
  auto [stage1, stage2] = build_page_table<std::uint16_t>(std::span{packed});
  return {std::move(blob), std::move(stage1), std::move(stage2)};
}

auto build_pages(const std::vector<PropertyEntry> &entries) -> TwoStageTable {
  std::vector<std::uint8_t> values(TOTAL_CODEPOINTS, 0);
  for (const auto &entry : entries) {
    for (std::uint32_t codepoint{entry.first}; codepoint <= entry.last;
         codepoint += 1) {
      values[codepoint] = entry.value;
    }
  }
  auto [stage1, stage2] = build_page_table<std::uint8_t>(std::span{values});
  return {std::move(stage1), std::move(stage2)};
}

template <typename T, typename Format>
auto emit_row(std::ostream &stream, const std::span<const T> items,
              const std::size_t row_width, Format format) -> void {
  for (std::size_t offset{0}; offset < items.size(); offset += row_width) {
    stream << "    ";
    const auto upper{offset + row_width < items.size() ? offset + row_width
                                                       : items.size()};
    for (std::size_t column{offset}; column < upper; column += 1) {
      if (column > offset) {
        stream << ", ";
      }
      format(stream, items[column]);
    }
    stream << ",\n";
  }
}

constexpr auto emit_decimal{[](std::ostream &stream, const auto value) {
  stream << static_cast<std::uint64_t>(value);
}};

constexpr auto emit_hex{[](std::ostream &stream, const auto value) {
  stream << "0x" << std::hex << std::uppercase
         << static_cast<std::uint64_t>(value) << std::dec;
}};

auto emit_property(std::ostream &stream, const std::string_view prefix,
                   const TwoStageTable &table) -> void {
  stream << "constexpr std::uint16_t " << prefix << "_STAGE1["
         << table.stage1.size() << "] = {\n";
  emit_row<std::uint16_t>(stream, table.stage1, 16, emit_decimal);
  stream << "};\n\n";
  stream << "constexpr std::uint8_t " << prefix << "_STAGE2["
         << table.stage2.size() << "] = {\n";
  emit_row<std::uint8_t>(stream, table.stage2, 16, emit_decimal);
  stream << "};\n\n";
}

auto emit_canonical_decomposition(std::ostream &stream,
                                  const DecompositionTable &table) -> void {
  stream << "constexpr char32_t CANONICAL_DECOMPOSITION_BLOB["
         << table.blob.size() << "] = {\n";
  emit_row<char32_t>(stream, table.blob, 8, emit_hex);
  stream << "};\n\n";
  stream << "constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE1["
         << table.stage1.size() << "] = {\n";
  emit_row<std::uint16_t>(stream, table.stage1, 16, emit_decimal);
  stream << "};\n\n";
  stream << "constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE2["
         << table.stage2.size() << "] = {\n";
  emit_row<std::uint16_t>(stream, table.stage2, 16, emit_decimal);
  stream << "};\n\n";
}

auto emit_canonical_composition(
    std::ostream &stream,
    const std::vector<CanonicalCompositionTriple> &triples) -> void {
  stream << "struct CanonicalCompositionEntry {\n"
         << "  char32_t starter;\n"
         << "  char32_t combining;\n"
         << "  char32_t composed;\n"
         << "};\n\n"
         << "constexpr CanonicalCompositionEntry CANONICAL_COMPOSITIONS["
         << triples.size() << "] = {\n";
  emit_row<CanonicalCompositionTriple>(
      stream, triples, 1,
      [](std::ostream &output, const CanonicalCompositionTriple &triple) {
        output << "{0x" << std::hex << std::uppercase
               << static_cast<std::uint64_t>(triple.starter) << ", 0x"
               << static_cast<std::uint64_t>(triple.combining) << ", 0x"
               << static_cast<std::uint64_t>(triple.composed) << std::dec
               << "}";
      });
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
    const std::filesystem::path normalization_props_path{positional.at(7)};
    const std::filesystem::path unicode_data_path{positional.at(8)};
    const std::filesystem::path composition_exclusions_path{positional.at(9)};

    const auto canonical_value{
        [](const std::span<const std::string_view> order) {
          return [order](const std::span<const std::string_view> aliases) {
            return canonical_index(order, aliases);
          };
        }};

    const auto combining_class_map{build_alias_map(
        aliases_path, "ccc",
        [](const std::span<const std::string_view> aliases) {
          const auto parsed{sourcemeta::core::to_uint32_t(aliases.front())};
          if (!parsed.has_value() || parsed.value() > 0xFFU) {
            throw std::runtime_error{
                std::string{"Invalid integer property value: "}.append(
                    aliases.front())};
          }
          return static_cast<std::uint8_t>(parsed.value());
        })};
    const auto joining_type_map{build_alias_map(
        aliases_path, "jt", canonical_value(JOINING_TYPE_ORDER))};
    const auto bidi_class_map{
        build_alias_map(aliases_path, "bc", canonical_value(BIDI_CLASS_ORDER))};
    const auto script_map{build_alias_map(
        aliases_path, "sc", canonical_value(UNICODE_SCRIPT_ORDER))};
    const auto combining_mark_map{
        build_alias_map(aliases_path, "gc",
                        [](const std::span<const std::string_view> aliases) {
                          for (const auto alias : aliases) {
                            if (alias == "M" || alias == "Mn" ||
                                alias == "Mc" || alias == "Me") {
                              return std::uint8_t{1};
                            }
                          }
                          return std::uint8_t{0};
                        })};
    const auto nfc_quick_check_map{build_alias_map(
        aliases_path, "NFC_QC", canonical_value(NFC_QUICK_CHECK_ORDER))};

    struct PropertySpec {
      std::string_view prefix;
      std::filesystem::path input_path;
      std::optional<std::string_view> property_filter;
      const ValueMap &value_map;
    };

    const std::array<PropertySpec, 6> properties{
        {{"COMBINING_CLASS", positional.at(2), std::nullopt,
          combining_class_map},
         {"JOINING_TYPE", positional.at(3), std::nullopt, joining_type_map},
         {"BIDI_CLASS", positional.at(4), std::nullopt, bidi_class_map},
         {"UNICODE_SCRIPT", positional.at(5), std::nullopt, script_map},
         {"IS_COMBINING_MARK", positional.at(6), std::nullopt,
          combining_mark_map},
         {"NFC_QUICK_CHECK", normalization_props_path,
          std::optional<std::string_view>{"NFC_QC"}, nfc_quick_check_map}}};

    const auto unicode_data{parse_unicode_data(unicode_data_path)};
    const auto full_exclusions{
        parse_full_composition_exclusions(normalization_props_path)};
    const auto explicit_exclusions{
        parse_explicit_composition_exclusions(composition_exclusions_path)};

    sourcemeta::core::write_file(output_path, [&](std::ostream &stream) {
      stream << "#include <cstddef>\n";
      stream << "#include <cstdint>\n\n";
      stream << "namespace {\n\n";
      for (const auto &spec : properties) {
        emit_property(
            stream, spec.prefix,
            build_pages(parse_property_file(spec.input_path, spec.value_map,
                                            spec.property_filter)));
      }
      emit_canonical_decomposition(stream, build_canonical_decomposition_pages(
                                               unicode_data.decompositions));
      emit_canonical_composition(stream, build_canonical_compositions(
                                             unicode_data.decompositions,
                                             unicode_data.ccc, full_exclusions,
                                             explicit_exclusions));
      stream << "} // namespace\n";
    });
  } catch (const std::exception &error) {
    std::cerr << "codegen: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
