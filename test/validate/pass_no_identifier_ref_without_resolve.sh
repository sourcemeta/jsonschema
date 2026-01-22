#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$ref": "./schemas/other.json"
}
EOF

mkdir -p "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/other.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo bar"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
annotation: "Test schema"
  at instance location "" (line 1, column 1)
  at evaluate path "/\$ref/description"
annotation: "Test"
  at instance location "" (line 1, column 1)
  at evaluate path "/\$ref/title"
annotation: "Test schema"
  at instance location "" (line 1, column 1)
  at evaluate path "/description"
annotation: "Test"
  at instance location "" (line 1, column 1)
  at evaluate path "/title"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
