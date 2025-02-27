#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << EOF > "$TMP/schema.json"
{
  "\$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object",
  "properties": {
    "nested": {
      "type": "object",
      "properties": {
        "foo": { "type": "string" }
      }
    }
  }
}
EOF

cat << EOF > "$TMP/instance.json"
{
  "foo": "this top-level property isn't relevant",
  "nested": {
    "foo": "hello world"
  }
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --path=/properties/nested \
  2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"

test "$CODE" = "0" || exit 1

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
