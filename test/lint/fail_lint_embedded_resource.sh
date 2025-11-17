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
      "allOf": [ { "$ref": "#/definitions/foo" } ],
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

cat << 'EOF' > "$TMP/expected.txt"
schema.json:10:7:
  `definitions` was superseded by `$defs` in 2019-09 and later versions (definitions_to_defs)
    at location "/$defs/embedded/definitions"
schema.json:9:20:
  Wrapping `$ref` in `allOf` was only necessary in JSON Schema Draft 7 and older (unnecessary_allof_ref_wrapper_modern)
    at location "/$defs/embedded/allOf/0/$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
