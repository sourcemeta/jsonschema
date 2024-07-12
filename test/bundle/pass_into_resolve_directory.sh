#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

mkdir "$TMP/schemas"
cat << 'EOF' > "$TMP/schemas/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/schemas" > "$TMP/schemas/result.json"

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

diff "$TMP/schemas/result.json" "$TMP/expected.json"

# Must come out formatted
"$1" fmt "$TMP/schemas/result.json" --check
