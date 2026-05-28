#!/usr/bin/env python3

import re
import sys

LINE = re.compile(r"^([0-9A-Fa-f]+)(?:\.\.([0-9A-Fa-f]+))?\s*;\s*(\S+)")
MISSING_PREFIX = re.compile(r"^#\s*@missing:\s*")

TOTAL_CODEPOINTS = 0x110000
PAGE_SHIFT = 10
PAGE_SIZE = 1 << PAGE_SHIFT
NUM_PAGES = TOTAL_CODEPOINTS // PAGE_SIZE

# Integer values must match the IDNAProperty enum in idna_ucd.h.
IDNA_PROPERTY_VALUES = {
    "PVALID": 0,
    "CONTEXTJ": 1,
    "CONTEXTO": 2,
    "DISALLOWED": 3,
    "UNASSIGNED": 4,
}


def parse_file(path, value_map):
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
            match = LINE.match(stripped)
            if not match:
                raise ValueError(
                    f"{path}:{line_number}: unparseable line: {stripped!r}"
                )
            first = int(match.group(1), 16)
            last = int(match.group(2), 16) if match.group(2) else first
            raw_value = match.group(3)
            try:
                value = value_map[raw_value]
            except KeyError as error:
                raise ValueError(
                    f"{path}:{line_number}: invalid value {raw_value!r}: {error}"
                ) from error
            target.append((first, last, value))
    return missing + data


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
    if len(sys.argv) != 3:
        print(
            f"Usage: {sys.argv[0]} <output.h> <Idna2008.txt>",
            file=sys.stderr,
        )
        sys.exit(1)

    output_path = sys.argv[1]
    idna_property_input = sys.argv[2]

    with open(output_path, "w") as output:
        output.write("#include <cstdint>\n\n")
        output.write("namespace {\n\n")
        stage1, pages = build_pages(
            parse_file(idna_property_input, IDNA_PROPERTY_VALUES)
        )
        emit_property(output, "IDNA_PROPERTY", stage1, pages)
        output.write("} // namespace\n")


if __name__ == "__main__":
    main()
