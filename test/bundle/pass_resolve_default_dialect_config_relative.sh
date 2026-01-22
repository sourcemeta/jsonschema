#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "title": "Test",
  "description": "Test schema",
  "$ref": "nested"
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$id": "https://example.com/nested",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"

"$1" bundle schema.json --resolve remote.json --verbose > "$TMP/result.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$id": "https://example.com",
  "title": "Test",
  "description": "Test schema",
  "$ref": "nested",
  "$defs": {
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/nested",
      "title": "Test",
      "description": "Test schema",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
