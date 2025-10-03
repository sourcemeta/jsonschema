#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "properties": {
    "foo": {
      "type": "string",
      "default": 1
    }
  }
}
EOF

cd "$TMP"
"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
schema.json:6:7:
  Only set a \`default\` value that validates against the schema (blaze/valid_default)
    at schema location "/properties/foo/default"
    The value was expected to be of type string but it was of type integer
      at instance location ""
      at evaluate path "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
