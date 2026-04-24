#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/metaschema.json"
{
  "$id": "https://example.com/custom-metaschema",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$vocabulary": { "https://json-schema.org/draft/2020-12/vocab/core": true },
  "$anchor": "foo",
  "$dynamicAnchor": "meta",
  "$ref": "https://json-schema.org/draft/2020-12/meta/core",
  "properties": {
    "bar": {
      "$anchor": "foo"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "type": "string"
}
EOF

"$1" metaschema "$TMP/schema.json" --resolve "$TMP/metaschema.json" 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Schema anchor already exists
  at identifier https://example.com/custom-metaschema#foo
  at line 9
  at column 5
  at file path $(realpath "$TMP")/metaschema.json
  at location "/properties/bar"
  at other location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" metaschema "$TMP/schema.json" --resolve "$TMP/metaschema.json" --json > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Schema anchor already exists",
  "identifier": "https://example.com/custom-metaschema#foo",
  "line": 9,
  "column": 5,
  "filePath": "$(realpath "$TMP")/metaschema.json",
  "location": "/properties/bar",
  "otherLocation": ""
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
