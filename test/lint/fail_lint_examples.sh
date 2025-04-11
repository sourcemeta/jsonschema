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
      "examples": [ 1, 2, 3 ]
    }
  }
}
EOF

"$1" lint "$TMP/schema.json" >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/schema.json:
  Only include instances in the \`examples\` array that validate against the schema (blaze/valid_examples)
    at schema location "/properties/foo"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
