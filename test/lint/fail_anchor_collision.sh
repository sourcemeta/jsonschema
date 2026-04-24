#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$anchor": "foo",
  "properties": {
    "bar": {
      "$anchor": "foo"
    }
  }
}
EOF

"$1" lint "$TMP/schema.json" 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Schema anchor already exists
  at identifier file://$(realpath "$TMP")/schema.json#foo
  at line 5
  at column 5
  at file path $(realpath "$TMP")/schema.json
  at location "/properties/bar"
  at other location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --json > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Schema anchor already exists",
  "identifier": "file://$(realpath "$TMP")/schema.json#foo",
  "line": 5,
  "column": 5,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/properties/bar",
  "otherLocation": ""
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
