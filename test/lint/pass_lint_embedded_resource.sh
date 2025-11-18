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
"$1" lint "$TMP/schema.json"
