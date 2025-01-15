#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com
$ref: nested
EOF

cat << 'EOF' > "$TMP/remote.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com/nested
type: string
EOF

"$1" bundle "$TMP/schema.yaml" --resolve "$TMP/remote.yaml" > "$TMP/result.json"

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

# Must come out formatted
"$1" fmt "$TMP/result.json" --check
