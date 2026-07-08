#include <sourcemeta/core/idna_ucd.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/text.h>

#include <algorithm> // std::sort
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>     // std::exception
#include <filesystem>    // std::filesystem::path
#include <iostream>      // std::cerr
#include <ostream>       // std::ostream
#include <ranges>        // std::views::transform
#include <span>          // std::span
#include <stdexcept>     // std::runtime_error
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace {

constexpr std::size_t TOTAL_CODEPOINTS{0x110000};
constexpr std::size_t TABLE_PAGE_SHIFT{10};
constexpr std::size_t TABLE_PAGE_SIZE{1 << TABLE_PAGE_SHIFT};
constexpr std::size_t NUM_PAGES{TOTAL_CODEPOINTS / TABLE_PAGE_SIZE};

struct PropertyEntry {
  std::uint32_t first;
  std::uint32_t last;
  sourcemeta::core::IDNAProperty value;
};

struct TwoStageTable {
  std::vector<std::uint16_t> stage1;
  std::vector<std::uint8_t> stage2;
};

auto property_from_token(const std::string_view token)
    -> sourcemeta::core::IDNAProperty {
#define SOURCEMETA_CORE_IDNA_PROPERTY_CASE(name, alias)                        \
  if (token == alias) {                                                        \
    return sourcemeta::core::IDNAProperty::name;                               \
  }
  SOURCEMETA_CORE_IDNA_PROPERTY_LIST(SOURCEMETA_CORE_IDNA_PROPERTY_CASE)
#undef SOURCEMETA_CORE_IDNA_PROPERTY_CASE
  throw std::runtime_error{
      std::string{"Unknown IDNA property value: "}.append(token)};
}

auto parse_hex_codepoint(const std::string_view token) -> std::uint32_t {
  const auto parsed{sourcemeta::core::to_uint32_t(token, 16)};
  if (!parsed.has_value() || parsed.value() > 0x10FFFF) {
    throw std::runtime_error{std::string{"Invalid codepoint: "}.append(token)};
  }
  return parsed.value();
}

auto parse_entry(const std::string_view payload) -> PropertyEntry {
  const auto trimmed{
      sourcemeta::core::trim(sourcemeta::core::take_until(payload, '#'))};
  const auto parts{sourcemeta::core::split_once(trimmed, ';')};
  if (!parts.has_value()) {
    throw std::runtime_error{
        std::string{"Missing ';' in line: "}.append(payload)};
  }
  const auto range_part{sourcemeta::core::trim(parts->first)};
  const auto value_part{sourcemeta::core::trim(parts->second)};
  const auto range_split{
      sourcemeta::core::split_once(range_part, std::string_view{".."})};
  const auto first{parse_hex_codepoint(
      range_split.has_value() ? range_split->first : range_part)};
  const auto last{range_split.has_value()
                      ? parse_hex_codepoint(range_split->second)
                      : first};
  return {first, last, property_from_token(value_part)};
}

auto parse_idna_file(const std::filesystem::path &input_path)
    -> std::vector<PropertyEntry> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::vector<PropertyEntry> missing;
  std::vector<PropertyEntry> data;
  constexpr std::string_view missing_prefix{"@missing:"};
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
      missing.push_back(
          parse_entry(comment_body.substr(missing_prefix.size())));
      return;
    }
    data.push_back(parse_entry(line));
  });
  std::vector<PropertyEntry> result;
  result.reserve(missing.size() + data.size());
  result.insert(result.end(), missing.begin(), missing.end());
  result.insert(result.end(), data.begin(), data.end());
  return result;
}

auto compress_pages(const std::vector<std::uint8_t> &values) -> TwoStageTable {
  std::unordered_map<std::string, std::uint16_t> page_to_id;
  TwoStageTable table;
  table.stage1.reserve(NUM_PAGES);
  for (std::size_t page_index{0}; page_index < NUM_PAGES; page_index += 1) {
    const auto page_start{page_index * TABLE_PAGE_SIZE};
    const std::string page_key{
        reinterpret_cast<const char *>(values.data() + page_start),
        TABLE_PAGE_SIZE};
    const auto existing{page_to_id.find(page_key)};
    if (existing != page_to_id.end()) {
      table.stage1.push_back(existing->second);
      continue;
    }
    const auto new_id{
        static_cast<std::uint16_t>(table.stage2.size() / TABLE_PAGE_SIZE)};
    page_to_id.emplace(page_key, new_id);
    table.stage2.insert(
        table.stage2.end(),
        values.begin() + static_cast<std::ptrdiff_t>(page_start),
        values.begin() +
            static_cast<std::ptrdiff_t>(page_start + TABLE_PAGE_SIZE));
    table.stage1.push_back(new_id);
  }
  return table;
}

auto build_pages(const std::vector<PropertyEntry> &entries) -> TwoStageTable {
  // RFC 5892 Section 2.10: a code point not covered by the derived-property
  // data defaults to Unassigned (which validation treats as disallowed), so an
  // uncovered code point fails closed rather than being accepted
  std::vector<std::uint8_t> values(
      TOTAL_CODEPOINTS,
      static_cast<std::uint8_t>(sourcemeta::core::IDNAProperty::Unassigned));
  for (const auto &entry : entries) {
    for (std::uint32_t codepoint{entry.first}; codepoint <= entry.last;
         codepoint += 1) {
      values[codepoint] = static_cast<std::uint8_t>(entry.value);
    }
  }
  return compress_pages(values);
}

template <typename T>
auto emit_row(std::ostream &stream, const std::span<const T> items) -> void {
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

auto emit_property(std::ostream &stream, const std::string_view prefix,
                   const TwoStageTable &table) -> void {
  stream << "constexpr std::uint16_t " << prefix << "_STAGE1["
         << table.stage1.size() << "] = {\n";
  emit_row<std::uint16_t>(stream, table.stage1);
  stream << "};\n\n";
  stream << "constexpr std::uint8_t " << prefix << "_STAGE2["
         << table.stage2.size() << "] = {\n";
  emit_row<std::uint8_t>(stream, table.stage2);
  stream << "};\n\n";
}

struct MappingEntry {
  std::uint32_t first;
  std::uint32_t last;
  sourcemeta::core::IDNAMappingStatus status;
  std::u32string mapping;
};

struct MappingIndexEntry {
  std::uint32_t codepoint;
  std::uint32_t offset;
  std::uint32_t length;
};

struct MappingTable {
  TwoStageTable status;
  std::u32string pool;
  std::vector<MappingIndexEntry> index;
};

auto mapping_status_from_token(const std::string_view token)
    -> sourcemeta::core::IDNAMappingStatus {
#define SOURCEMETA_CORE_IDNA_MAPPING_CASE(name, alias)                         \
  if (token == alias) {                                                        \
    return sourcemeta::core::IDNAMappingStatus::name;                          \
  }
  SOURCEMETA_CORE_IDNA_MAPPING_STATUS_LIST(SOURCEMETA_CORE_IDNA_MAPPING_CASE)
#undef SOURCEMETA_CORE_IDNA_MAPPING_CASE
  throw std::runtime_error{
      std::string{"Unknown IDNA mapping status: "}.append(token)};
}

// Parse a space-separated list of hexadecimal codepoints (the mapping column)
auto parse_mapping_sequence(const std::string_view field) -> std::u32string {
  std::u32string result;
  sourcemeta::core::split(field, ' ', [&](const std::string_view token) {
    const auto trimmed{sourcemeta::core::trim(token)};
    if (!trimmed.empty()) {
      result.push_back(static_cast<char32_t>(parse_hex_codepoint(trimmed)));
    }
  });
  return result;
}

auto parse_mapping_line(const std::string_view payload) -> MappingEntry {
  const auto trimmed{
      sourcemeta::core::trim(sourcemeta::core::take_until(payload, '#'))};
  const auto first_split{sourcemeta::core::split_once(trimmed, ';')};
  if (!first_split.has_value()) {
    throw std::runtime_error{
        std::string{"Missing ';' in line: "}.append(payload)};
  }
  const auto range_part{sourcemeta::core::trim(first_split->first)};
  const auto range_split{
      sourcemeta::core::split_once(range_part, std::string_view{".."})};
  const auto first{parse_hex_codepoint(
      range_split.has_value() ? range_split->first : range_part)};
  const auto last{range_split.has_value()
                      ? parse_hex_codepoint(range_split->second)
                      : first};

  const auto second_split{
      sourcemeta::core::split_once(first_split->second, ';')};
  const auto status_part{sourcemeta::core::trim(
      second_split.has_value() ? second_split->first : first_split->second)};
  const auto status{mapping_status_from_token(status_part)};

  std::u32string mapping;
  if (status == sourcemeta::core::IDNAMappingStatus::Mapped &&
      second_split.has_value()) {
    const auto third_split{
        sourcemeta::core::split_once(second_split->second, ';')};
    const auto mapping_part{sourcemeta::core::trim(
        third_split.has_value() ? third_split->first : second_split->second)};
    mapping = parse_mapping_sequence(mapping_part);
  }

  return {first, last, status, std::move(mapping)};
}

auto parse_mapping_file(const std::filesystem::path &input_path)
    -> std::vector<MappingEntry> {
  auto stream{sourcemeta::core::read_file(input_path)};
  std::vector<MappingEntry> entries;
  sourcemeta::core::for_each_line(stream, [&](const std::string_view raw_line) {
    const auto line{sourcemeta::core::trim(raw_line)};
    if (line.empty() || line.front() == '#') {
      return;
    }
    entries.push_back(parse_mapping_line(line));
  });
  return entries;
}

auto build_mapping(const std::vector<MappingEntry> &entries) -> MappingTable {
  // Codepoints absent from the table default to disallowed per UTS #46
  std::vector<std::uint8_t> status_values(
      TOTAL_CODEPOINTS, static_cast<std::uint8_t>(
                            sourcemeta::core::IDNAMappingStatus::Disallowed));
  MappingTable table;
  // Deduplicate identical replacement sequences into a shared pool
  std::unordered_map<std::u32string, std::uint32_t> sequence_to_offset;
  for (const auto &entry : entries) {
    std::uint32_t offset{0};
    if (entry.status == sourcemeta::core::IDNAMappingStatus::Mapped) {
      const auto existing{sequence_to_offset.find(entry.mapping)};
      if (existing != sequence_to_offset.end()) {
        offset = existing->second;
      } else {
        offset = static_cast<std::uint32_t>(table.pool.size());
        sequence_to_offset.emplace(entry.mapping, offset);
        table.pool.append(entry.mapping);
      }
    }
    for (std::uint32_t codepoint{entry.first}; codepoint <= entry.last;
         codepoint += 1) {
      status_values[codepoint] = static_cast<std::uint8_t>(entry.status);
      if (entry.status == sourcemeta::core::IDNAMappingStatus::Mapped) {
        table.index.push_back(
            {codepoint, offset,
             static_cast<std::uint32_t>(entry.mapping.size())});
      }
    }
  }

  std::sort(table.index.begin(), table.index.end(),
            [](const MappingIndexEntry &left, const MappingIndexEntry &right) {
              return left.codepoint < right.codepoint;
            });
  table.status = compress_pages(status_values);
  return table;
}

auto emit_mapping(std::ostream &stream, const MappingTable &table) -> void {
  emit_property(stream, "IDNA_MAPPING_STATUS", table.status);

  stream << "constexpr char32_t IDNA_MAPPING_POOL[" << table.pool.size()
         << "] = {\n";
  emit_row<char32_t>(
      stream, std::span<const char32_t>{table.pool.data(), table.pool.size()});
  stream << "};\n\n";

  stream << "struct IDNAMappingEntry {\n";
  stream << "  std::uint32_t codepoint;\n";
  stream << "  std::uint32_t offset;\n";
  stream << "  std::uint32_t length;\n";
  stream << "};\n\n";
  stream << "constexpr IDNAMappingEntry IDNA_MAPPING_INDEX["
         << table.index.size() << "] = {\n";
  for (const auto &entry : table.index) {
    stream << "    {" << entry.codepoint << ", " << entry.offset << ", "
           << entry.length << "},\n";
  }
  stream << "};\n\n";
}

} // namespace

auto main(const int argc, const char *const argv[]) -> int {
  try {
    sourcemeta::core::Options app;
    app.parse(argc, argv);
    const auto &positional{app.positional()};
    if (positional.size() != 3) {
      std::cerr << "Usage: " << (argc > 0 ? argv[0] : "codegen")
                << " <output.h> <Idna2008.txt> <IdnaMappingTable.txt>\n";
      return EXIT_FAILURE;
    }

    const std::filesystem::path output_path{positional.at(0)};
    const std::filesystem::path property_path{positional.at(1)};
    const std::filesystem::path mapping_path{positional.at(2)};

    const auto table{build_pages(parse_idna_file(property_path))};
    const auto mapping{build_mapping(parse_mapping_file(mapping_path))};
    sourcemeta::core::write_file(output_path, [&](std::ostream &stream) {
      stream << "#include <cstdint>\n\n";
      stream << "namespace {\n\n";
      emit_property(stream, "IDNA_PROPERTY", table);
      emit_mapping(stream, mapping);
      stream << "} // namespace\n";
    });
  } catch (const std::exception &error) {
    std::cerr << "codegen: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
