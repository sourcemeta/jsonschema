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
    "foo": { "type": "string" },
    "bar": "I am not a schema, I'm just a string"
  }
}
EOF

cat << EOF > "$TMP/instance.json"
{
  "bar": "some value"
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --path=/properties/bar \
  2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"

test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The target of the reference is not a valid schema
  #/properties/bar
    at schema location "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
