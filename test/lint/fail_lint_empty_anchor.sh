#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$anchor": ""
}
EOF

"$1" lint "$TMP/schema.json" > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Schema anchor already exists
  at identifier file://$(realpath "$TMP")/schema.json
  at line 1
  at column 1
  at file path $(realpath "$TMP")/schema.json
  at location ""
  at other location ""
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --json > "$TMP/output.json" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.json"
{
  "error": "Schema anchor already exists",
  "identifier": "file://$(realpath "$TMP")/schema.json",
  "line": 1,
  "column": 1,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "",
  "otherLocation": ""
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
