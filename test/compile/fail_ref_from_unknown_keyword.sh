#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "properties": {
    "foo": { "$ref": "#/$defs/bar" },
    "baz": { "type": "string" }
  },
  "$defs": {
    "bar": {
      "$ref": "#/properties/baz"
    }
  }
}
EOF

"$1" compile "$TMP/schema.json" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The schema location is inside of an unknown keyword
  #/properties/baz
    at schema location "/$defs/bar/$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
