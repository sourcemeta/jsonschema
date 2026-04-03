#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "$id": "https://example.com",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": { "$ref": "https://example.com/nested" }
  }
}
EOF

cat << 'EOF' > "$TMP/nested.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/nested",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/nested.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "$id": "https://example.com",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "$ref": "https://example.com/nested"
    }
  },
  "definitions": {
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft-07/schema#",
      "$id": "https://example.com/nested",
      "title": "Test",
      "description": "Test schema",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
