#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/meta.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/meta",
  "$vocabulary": {
    "https://json-schema.org/draft/2019-09/vocab/core": true,
    "https://json-schema.org/draft/2019-09/vocab/applicator": true,
    "https://json-schema.org/draft/2019-09/vocab/validation": true,
    "https://json-schema.org/draft/2019-09/vocab/meta-data": true,
    "https://json-schema.org/draft/2019-09/vocab/format": false,
    "https://json-schema.org/draft/2019-09/vocab/content": true
  }
}
EOF

cat << 'EOF' > "$TMP/schemas/remote.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/schemas" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com",
  "$ref": "nested",
  "$defs": {
    "https://example.com/meta": {
      "$schema": "https://json-schema.org/draft/2019-09/schema",
      "$id": "https://example.com/meta",
      "$vocabulary": {
        "https://json-schema.org/draft/2019-09/vocab/applicator": true,
        "https://json-schema.org/draft/2019-09/vocab/content": true,
        "https://json-schema.org/draft/2019-09/vocab/core": true,
        "https://json-schema.org/draft/2019-09/vocab/format": false,
        "https://json-schema.org/draft/2019-09/vocab/meta-data": true,
        "https://json-schema.org/draft/2019-09/vocab/validation": true
      }
    },
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft/2019-09/schema",
      "$id": "https://example.com/nested",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

# Must come out formatted
"$1" fmt "$TMP/result.json" --check
