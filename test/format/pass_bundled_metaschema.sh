#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/schema",
  "$schema": "https://example.com/meta",
  "$defs": {
    "https://example.com/meta": {
      "$id": "https://example.com/meta",
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      },
      "type": "object"
    }
  },
  "type": "string"
}
EOF

"$1" fmt "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com/schema",
  "type": "string",
  "$defs": {
    "https://example.com/meta": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/meta",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      },
      "type": "object"
    }
  }
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"

"$1" fmt "$TMP/schema.json" --check > "$TMP/check_output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_check_output.txt"
EOF

diff "$TMP/check_output.txt" "$TMP/expected_check_output.txt"
