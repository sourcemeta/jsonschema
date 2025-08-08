#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "./schemas/other.json"
}
EOF

mkdir -p "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo bar"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Attempting to read file reference from disk: $(realpath "$TMP")/schemas/other.json
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
