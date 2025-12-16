#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# This schema has no identifier
cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$ref": "./schemas/other.json"
}
EOF

mkdir -p "$TMP/schemas"

# But the target does
cat << 'EOF' > "$TMP/schemas/other.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo bar"
EOF

"$1" validate "$TMP/schema.json" --resolve "$TMP/schemas" "$TMP/instance.json"
