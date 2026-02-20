#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/entry.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Entry",
  "$ref": "./bundled.json"
}
EOF

cat << 'EOF' > "$TMP/bundled.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/schemas/parent",
  "$ref": "./child",
  "$defs": {
    "https://example.com/schemas/child": {
      "$id": "https://example.com/schemas/child",
      "type": "string"
    }
  }
}
EOF

cat << EOF > "$TMP/expected.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "file://$(realpath "$TMP")/entry.json",
  "title": "Entry",
  "\$ref": "https://example.com/schemas/parent",
  "\$defs": {
    "https://example.com/schemas/parent": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "https://example.com/schemas/parent",
      "\$ref": "./child",
      "\$defs": {
        "https://example.com/schemas/child": {
          "\$id": "https://example.com/schemas/child",
          "type": "string"
        }
      }
    }
  }
}
EOF

"$1" bundle "$TMP/entry.json" > "$TMP/result.json"

diff "$TMP/result.json" "$TMP/expected.json"
