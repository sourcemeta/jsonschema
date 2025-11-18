#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "embedded",
  "$defs": {
    "embedded": {
      "$schema": "http://json-schema.org/draft-07/schema#",
      "$id": "embedded",
      "$ref": "#/definitions/foo",
      "definitions": {
        "foo": { "type": "number" }
      }
    }
  }
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
schema.json:10:7:
  `definitions` was superseded by `$defs` in 2019-09 and later versions (definitions_to_defs)
    at location "/$defs/embedded/definitions"
schema.json:7:7:
  A `$schema` declaration without a sibling identifier (or with a sibling `$ref` in Draft 7 and older dialects), is ignored (ignored_metaschema)
    at location "/$defs/embedded/$schema"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
