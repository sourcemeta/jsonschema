#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com/anchor-collision-schema",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$anchor": "foo",
  "properties": {
    "bar": {
      "$anchor": "foo"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com/anchor-collision-schema",
  "tests": [
    {
      "description": "Test case",
      "valid": true,
      "data": "hello"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Schema anchor already exists
  at identifier https://example.com/anchor-collision-schema#foo
  at line 6
  at column 5
  at file path $(realpath "$TMP")/schema.json
  at location "/properties/bar"
  at other location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --json > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Schema anchor already exists",
  "identifier": "https://example.com/anchor-collision-schema#foo",
  "line": 6,
  "column": 5,
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/properties/bar",
  "otherLocation": ""
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
