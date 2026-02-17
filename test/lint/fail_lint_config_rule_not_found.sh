#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [
      "./nonexistent.json"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: No such file or directory
  at file path $(realpath "$TMP")/nonexistent.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP"
"$1" lint --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "No such file or directory",
  "filePath": "$(realpath "$TMP")/nonexistent.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
