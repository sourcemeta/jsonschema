#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/schema",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "$defs": {
    "PositiveInt": {
      "type": "integer",
      "minimum": 1
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
42
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint "https://example.com/schema#/\$defs/PositiveInt" \
  --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
