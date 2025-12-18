#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com
$ref: nested
EOF

cat << 'EOF' > "$TMP/nested.custom"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com/nested
type: string
EOF

"$1" bundle "$TMP/schema.custom" --resolve "$TMP/nested.custom" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "nested",
  "$defs": {
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/nested",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
