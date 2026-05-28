#!/usr/bin/env python3

import re
import sys

LINE = re.compile(r"^([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(\S+)")
MULTI_PROPERTY_LINE = re.compile(
    r"^([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(\S+)\s*;\s*(\S+)"
)
# Boolean-property rows in multi-property files use a two-field shape,
# with no value column. Used to recognise the row instead of silently
# skipping it.
BOOLEAN_PROPERTY_LINE = re.compile(
    r"^([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(\S+)\s*$"
)
MISSING_PREFIX = re.compile(r"^#\s*@missing:\s*")

TOTAL_CODEPOINTS = 0x110000
PAGE_SHIFT = 10
PAGE_SIZE = 1 << PAGE_SHIFT
NUM_PAGES = TOTAL_CODEPOINTS // PAGE_SIZE

# Per-property canonical order. Position in this list defines the integer
# value of the matching C++ enum entry. PropertyValueAliases.txt supplies
# the short/long alias mappings at codegen time, so we only need to
# declare one form per value here.

JOINING_TYPE_ORDER = ["U", "T", "L", "R", "D", "C"]

BIDI_CLASS_ORDER = [
    "L", "R", "AL", "EN", "ES", "ET", "AN", "CS", "NSM", "BN",
    "B", "S", "WS", "ON", "LRE", "LRO", "RLE", "RLO", "PDF",
    "LRI", "RLI", "FSI", "PDI",
]

NFC_QUICK_CHECK_ORDER = ["Y", "N", "M"]

UNICODE_SCRIPT_ORDER = [
    "Adlam", "Ahom", "Anatolian_Hieroglyphs", "Arabic", "Armenian",
    "Avestan", "Balinese", "Bamum", "Bassa_Vah", "Batak", "Bengali",
    "Beria_Erfe", "Bhaiksuki", "Bopomofo", "Brahmi", "Braille",
    "Buginese", "Buhid", "Canadian_Aboriginal", "Carian",
    "Caucasian_Albanian", "Chakma", "Cham", "Cherokee", "Chorasmian",
    "Common", "Coptic", "Cuneiform", "Cypriot", "Cypro_Minoan",
    "Cyrillic", "Deseret", "Devanagari", "Dives_Akuru", "Dogra",
    "Duployan", "Egyptian_Hieroglyphs", "Elbasan", "Elymaic",
    "Ethiopic", "Garay", "Georgian", "Glagolitic", "Gothic", "Grantha",
    "Greek", "Gujarati", "Gunjala_Gondi", "Gurmukhi", "Gurung_Khema",
    "Han", "Hangul", "Hanifi_Rohingya", "Hanunoo", "Hatran", "Hebrew",
    "Hiragana", "Imperial_Aramaic", "Inherited", "Inscriptional_Pahlavi",
    "Inscriptional_Parthian", "Javanese", "Kaithi", "Kannada", "Katakana",
    "Kawi", "Kayah_Li", "Kharoshthi", "Khitan_Small_Script", "Khmer",
    "Khojki", "Khudawadi", "Kirat_Rai", "Lao", "Latin", "Lepcha", "Limbu",
    "Linear_A", "Linear_B", "Lisu", "Lycian", "Lydian", "Mahajani",
    "Makasar", "Malayalam", "Mandaic", "Manichaean", "Marchen",
    "Masaram_Gondi", "Medefaidrin", "Meetei_Mayek", "Mende_Kikakui",
    "Meroitic_Cursive", "Meroitic_Hieroglyphs", "Miao", "Modi",
    "Mongolian", "Mro", "Multani", "Myanmar", "Nabataean", "Nag_Mundari",
    "Nandinagari", "New_Tai_Lue", "Newa", "Nko", "Nushu",
    "Nyiakeng_Puachue_Hmong", "Ogham", "Ol_Chiki", "Ol_Onal",
    "Old_Hungarian", "Old_Italic", "Old_North_Arabian", "Old_Permic",
    "Old_Persian", "Old_Sogdian", "Old_South_Arabian", "Old_Turkic",
    "Old_Uyghur", "Oriya", "Osage", "Osmanya", "Pahawh_Hmong",
    "Palmyrene", "Pau_Cin_Hau", "Phags_Pa", "Phoenician",
    "Psalter_Pahlavi", "Rejang", "Runic", "Samaritan", "Saurashtra",
    "Sharada", "Shavian", "Siddham", "Sidetic", "SignWriting", "Sinhala",
    "Sogdian", "Sora_Sompeng", "Soyombo", "Sundanese", "Sunuwar",
    "Syloti_Nagri", "Syriac", "Tagalog", "Tagbanwa", "Tai_Le", "Tai_Tham",
    "Tai_Viet", "Tai_Yo", "Takri", "Tamil", "Tangsa", "Tangut", "Telugu",
    "Thaana", "Thai", "Tibetan", "Tifinagh", "Tirhuta", "Todhri",
    "Tolong_Siki", "Toto", "Tulu_Tigalari", "Ugaritic", "Unknown", "Vai",
    "Vithkuqi", "Wancho", "Warang_Citi", "Yezidi", "Yi", "Zanabazar_Square",
    "Katakana_Or_Hiragana",
]


def parse_alias_lines(aliases_path, property_short):
    rows = []
    with open(aliases_path) as source:
        for line in source:
            stripped = line.split("#", 1)[0].strip()
            if not stripped:
                continue
            parts = [part.strip() for part in stripped.split(";")]
            if parts[0] == property_short:
                rows.append([field for field in parts[1:] if field])
    return rows


def build_combining_mark_value_map(aliases_path):
    """Build {form: int} from PropertyValueAliases.txt mapping each
    General_Category alias to 1 if it is a combining mark (Mn, Mc, Me,
    or the supergroup M / Mark / Combining_Mark) and to 0 otherwise."""
    combining = {"M", "Mn", "Mc", "Me"}
    result = {}
    for row in parse_alias_lines(aliases_path, "gc"):
        value = 1 if any(field in combining for field in row) else 0
        for field in row:
            result[field] = value
    return result


def build_value_map(aliases_path, property_short, canonical_order=None):
    """Build {form: int} for a property. With canonical_order, each row's
    integer is its canonical's position in that list; without, the row's
    first field is read as the integer directly (used for ccc)."""
    canonical_to_int = (
        {name: index for index, name in enumerate(canonical_order)}
        if canonical_order is not None
        else None
    )
    result = {}
    unmatched = []
    for row in parse_alias_lines(aliases_path, property_short):
        if canonical_to_int is None:
            value = int(row[0])
        else:
            value = next(
                (canonical_to_int[field] for field in row if field in canonical_to_int),
                None,
            )
            if value is None:
                unmatched.append(row)
                continue
        for field in row:
            result[field] = value
    if unmatched:
        raise ValueError(
            f"{aliases_path}: property {property_short!r} has values not "
            f"declared in canonical order: {unmatched}"
        )
    return result


def parse_file(path, value_map, property_filter=None):
    """Read a UCD file and return a list of (first, last, value) entries
    with @missing defaults first and data ranges second, so callers can
    apply them in order regardless of where @missing appears in the file.

    With property_filter set, lines have shape `codepoint; property; value`
    (as in DerivedNormalizationProps.txt) and only rows whose property
    name matches are returned. Without it, lines have shape
    `codepoint; value` and every row contributes."""
    line_re = MULTI_PROPERTY_LINE if property_filter is not None else LINE
    missing = []
    data = []
    with open(path) as source:
        for line_number, line in enumerate(source, start=1):
            stripped = line.strip()
            if not stripped:
                continue
            target = data
            if stripped.startswith("#"):
                prefix = MISSING_PREFIX.match(stripped)
                if not prefix:
                    continue
                stripped = stripped[prefix.end():]
                target = missing
            match = line_re.match(stripped)
            if not match:
                # Recognise the boolean-property shape used in multi-property
                # files, but only for properties other than the one we are
                # filtering for. A boolean-shape row that names our target
                # property would be malformed data and must raise.
                data_only = stripped.split("#", 1)[0].strip()
                if property_filter is not None:
                    boolean = BOOLEAN_PROPERTY_LINE.fullmatch(data_only)
                    if boolean and boolean.group(3) != property_filter:
                        continue
                raise ValueError(
                    f"{path}:{line_number}: unparseable line: {stripped!r}"
                )
            if property_filter is not None and match.group(3) != property_filter:
                continue
            first = int(match.group(1), 16)
            last = int(match.group(2), 16) if match.group(2) else first
            raw_value = match.group(4 if property_filter is not None else 3)
            try:
                value = value_map[raw_value]
            except KeyError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid value {raw_value!r}: {error}"
                ) from error
            target.append((first, last, value))
    return missing + data


def parse_unicode_data(path):
    """Read UnicodeData.txt once and return (decompositions, ccc) where
    decompositions is {codepoint: [decomposition codepoints]} for canonical
    decompositions only (compatibility decompositions, those whose field 5
    starts with a `<tag>` prefix per UAX #44, are excluded), and ccc is
    {codepoint: canonical_combining_class} for codepoints with non-zero CCC.

    Raises if any canonical decomposition has more than two codepoints, which
    would indicate a format change in UnicodeData.txt."""
    decompositions = {}
    ccc = {}
    with open(path) as source:
        for line_number, line in enumerate(source, start=1):
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            fields = stripped.split(";")
            if len(fields) < 6:
                raise ValueError(
                    f"{path}:{line_number}: too few fields: {stripped!r}"
                )
            try:
                codepoint = int(fields[0], 16)
            except ValueError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid codepoint: {fields[0]!r}"
                ) from error
            try:
                ccc_value = int(fields[3])
            except ValueError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid CCC: {fields[3]!r}"
                ) from error
            if ccc_value != 0:
                ccc[codepoint] = ccc_value
            decomp_field = fields[5].strip()
            if not decomp_field or decomp_field.startswith("<"):
                continue
            decomposition = [int(token, 16) for token in decomp_field.split()]
            if len(decomposition) > 2:
                raise ValueError(
                    f"{path}:{line_number}: canonical decomposition of "
                    f"U+{codepoint:04X} has {len(decomposition)} codepoints, "
                    f"expected 1 or 2"
                )
            decompositions[codepoint] = decomposition
    return decompositions, ccc


def parse_full_composition_exclusions(path):
    """Read DerivedNormalizationProps.txt and return the set of codepoints
    for which Full_Composition_Exclusion=Yes. Lines that match neither the
    three-field nor the two-field property shape raise, so a file format
    change cannot silently drop exclusion data."""
    result = set()
    with open(path) as source:
        for line_number, line in enumerate(source, start=1):
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            match = MULTI_PROPERTY_LINE.match(stripped)
            if match:
                if match.group(3) != "Full_Composition_Exclusion":
                    continue
                first = int(match.group(1), 16)
                last = int(match.group(2), 16) if match.group(2) else first
                for codepoint in range(first, last + 1):
                    result.add(codepoint)
                continue
            data_only = stripped.split("#", 1)[0].strip()
            boolean_match = BOOLEAN_PROPERTY_LINE.fullmatch(data_only)
            if not boolean_match:
                raise ValueError(
                    f"{path}:{line_number}: unparseable line: {stripped!r}"
                )
            if boolean_match.group(3) != "Full_Composition_Exclusion":
                continue
            first = int(boolean_match.group(1), 16)
            last = (int(boolean_match.group(2), 16)
                    if boolean_match.group(2) else first)
            for codepoint in range(first, last + 1):
                result.add(codepoint)
    return result


EXPLICIT_COMPOSITION_EXCLUSION_LINE = re.compile(
    r"^([0-9A-Fa-f]+)(?:\s+#.*)?$"
)


def parse_explicit_composition_exclusions(path):
    """Read the script-specific list from CompositionExclusions.txt. The
    file has a flat `codepoint  # NAME` shape with no semicolons. The full
    line is anchored so prefix-only matches and trailing junk fail loud."""
    result = set()
    with open(path) as source:
        for line_number, line in enumerate(source, start=1):
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            match = EXPLICIT_COMPOSITION_EXCLUSION_LINE.fullmatch(stripped)
            if not match:
                raise ValueError(
                    f"{path}:{line_number}: unparseable line: {stripped!r}"
                )
            result.add(int(match.group(1), 16))
    return result


def build_canonical_compositions(decompositions, ccc, full_exclusions,
                                  explicit_exclusions):
    """Apply the UAX #15 §1.3 primary-composite filters and return a sorted
    list of (starter, combining, composed) triples.

    Raises if the explicit CompositionExclusions.txt list is not a subset of
    the derived Full_Composition_Exclusion set in
    DerivedNormalizationProps.txt, which would indicate the two data files
    have drifted out of sync."""
    missing = explicit_exclusions - full_exclusions
    if missing:
        raise ValueError(
            "CompositionExclusions.txt entries missing from "
            "Full_Composition_Exclusion: "
            + ", ".join(f"U+{codepoint:04X}" for codepoint in sorted(missing))
        )

    triples = []
    for composed, decomposition in decompositions.items():
        if len(decomposition) != 2:
            continue
        if ccc.get(composed, 0) != 0:
            continue
        if ccc.get(decomposition[0], 0) != 0:
            continue
        if composed in full_exclusions:
            continue
        triples.append((decomposition[0], decomposition[1], composed))
    triples.sort()
    return triples


def emit_canonical_composition(output, triples):
    output.write("struct CanonicalCompositionEntry {\n")
    output.write("  char32_t starter;\n")
    output.write("  char32_t combining;\n")
    output.write("  char32_t composed;\n")
    output.write("};\n\n")
    output.write(
        f"constexpr CanonicalCompositionEntry "
        f"CANONICAL_COMPOSITIONS[{len(triples)}] = {{\n"
    )
    for starter, combining, composed in triples:
        output.write(
            f"    {{0x{starter:X}, 0x{combining:X}, 0x{composed:X}}},\n"
        )
    output.write("};\n\n")


# Packed per-codepoint entry: (length << OFFSET_BITS) | offset. A zero entry
# means no decomposition. Length 1 / 2 covers the entire canonical space.
DECOMPOSITION_OFFSET_BITS = 14
DECOMPOSITION_OFFSET_MASK = (1 << DECOMPOSITION_OFFSET_BITS) - 1


def build_canonical_decomposition_pages(decompositions):
    """Build the flat blob plus per-codepoint packed entries, then run the
    standard two-stage page-table dedup on top of the packed array."""
    blob = []
    packed = [0] * TOTAL_CODEPOINTS
    for codepoint in sorted(decompositions):
        decomposition = decompositions[codepoint]
        offset = len(blob)
        if offset > DECOMPOSITION_OFFSET_MASK:
            raise ValueError(
                f"canonical decomposition blob exceeds "
                f"{DECOMPOSITION_OFFSET_BITS}-bit offset cap at "
                f"U+{codepoint:04X}"
            )
        blob.extend(decomposition)
        packed[codepoint] = (len(decomposition) << DECOMPOSITION_OFFSET_BITS) | offset

    page_to_id = {}
    unique_pages = []
    stage1 = []
    for page_index in range(NUM_PAGES):
        start = page_index * PAGE_SIZE
        page = tuple(packed[start : start + PAGE_SIZE])
        if page not in page_to_id:
            page_to_id[page] = len(unique_pages)
            unique_pages.append(page)
        stage1.append(page_to_id[page])
    return blob, stage1, unique_pages


def emit_canonical_decomposition(output, blob, stage1, unique_pages):
    output.write(
        f"constexpr char32_t CANONICAL_DECOMPOSITION_BLOB[{len(blob)}] = {{\n"
    )
    for offset in range(0, len(blob), 8):
        chunk = blob[offset : offset + 8]
        output.write(
            "    " + ", ".join(f"0x{value:X}" for value in chunk) + ",\n"
        )
    output.write("};\n\n")

    output.write(
        f"constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE1"
        f"[{len(stage1)}] = {{\n"
    )
    emit_row(output, stage1)
    output.write("};\n\n")
    stage2_size = len(unique_pages) * PAGE_SIZE
    output.write(
        f"constexpr std::uint16_t CANONICAL_DECOMPOSITION_STAGE2"
        f"[{stage2_size}] = {{\n"
    )
    for page in unique_pages:
        emit_row(output, list(page))
    output.write("};\n\n")


def build_pages(entries):
    values = [0] * TOTAL_CODEPOINTS
    for first, last, value in entries:
        values[first : last + 1] = [value] * (last - first + 1)
    page_to_id = {}
    unique_pages = []
    stage1 = []
    for page_index in range(NUM_PAGES):
        start = page_index * PAGE_SIZE
        page = tuple(values[start : start + PAGE_SIZE])
        if page not in page_to_id:
            page_to_id[page] = len(unique_pages)
            unique_pages.append(page)
        stage1.append(page_to_id[page])
    return stage1, unique_pages


def emit_row(output, items):
    for offset in range(0, len(items), 16):
        chunk = items[offset : offset + 16]
        output.write("    " + ", ".join(str(value) for value in chunk) + ",\n")


def emit_property(output, prefix, stage1, unique_pages):
    output.write(
        f"constexpr std::uint16_t {prefix}_STAGE1[{len(stage1)}] = {{\n"
    )
    emit_row(output, stage1)
    output.write("};\n\n")
    stage2_size = len(unique_pages) * PAGE_SIZE
    output.write(
        f"constexpr std::uint8_t {prefix}_STAGE2[{stage2_size}] = {{\n"
    )
    for page in unique_pages:
        emit_row(output, list(page))
    output.write("};\n\n")


def main():
    if len(sys.argv) != 11:
        print(
            f"Usage: {sys.argv[0]} "
            "<output.h> "
            "<PropertyValueAliases.txt> "
            "<DerivedCombiningClass.txt> "
            "<DerivedJoiningType.txt> "
            "<DerivedBidiClass.txt> "
            "<Scripts.txt> "
            "<DerivedGeneralCategory.txt> "
            "<DerivedNormalizationProps.txt> "
            "<UnicodeData.txt> "
            "<CompositionExclusions.txt>",
            file=sys.stderr,
        )
        sys.exit(1)

    output_path = sys.argv[1]
    aliases_path = sys.argv[2]
    derived_normalization_props_path = sys.argv[8]

    properties = [
        ("COMBINING_CLASS", sys.argv[3], None,
         build_value_map(aliases_path, "ccc")),
        ("JOINING_TYPE", sys.argv[4], None,
         build_value_map(aliases_path, "jt", JOINING_TYPE_ORDER)),
        ("BIDI_CLASS", sys.argv[5], None,
         build_value_map(aliases_path, "bc", BIDI_CLASS_ORDER)),
        ("UNICODE_SCRIPT", sys.argv[6], None,
         build_value_map(aliases_path, "sc", UNICODE_SCRIPT_ORDER)),
        ("IS_COMBINING_MARK", sys.argv[7], None,
         build_combining_mark_value_map(aliases_path)),
        ("NFC_QUICK_CHECK", derived_normalization_props_path, "NFC_QC",
         build_value_map(aliases_path, "NFC_QC", NFC_QUICK_CHECK_ORDER)),
    ]

    unicode_data_path = sys.argv[9]
    composition_exclusions_path = sys.argv[10]

    decompositions, ccc = parse_unicode_data(unicode_data_path)
    full_exclusions = parse_full_composition_exclusions(
        derived_normalization_props_path
    )
    explicit_exclusions = parse_explicit_composition_exclusions(
        composition_exclusions_path
    )

    with open(output_path, "w") as output:
        output.write("#include <cstddef>\n")
        output.write("#include <cstdint>\n\n")
        output.write("namespace {\n\n")
        for prefix, input_path, property_filter, value_map in properties:
            stage1, pages = build_pages(
                parse_file(input_path, value_map, property_filter)
            )
            emit_property(output, prefix, stage1, pages)
        blob, stage1, pages = build_canonical_decomposition_pages(
            decompositions
        )
        emit_canonical_decomposition(output, blob, stage1, pages)
        triples = build_canonical_compositions(
            decompositions, ccc, full_exclusions, explicit_exclusions
        )
        emit_canonical_composition(output, triples)
        output.write("} // namespace\n")


if __name__ == "__main__":
    main()
