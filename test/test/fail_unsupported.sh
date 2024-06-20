#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "schema": "https://json-schema.org/draft/2020-12/schema",
  "tests": [
    {
      "valid": true,
      "data": {}
    },
    {
      "valid": true,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --verbose 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: Cannot compile unsupported vocabulary
  https://json-schema.org/draft/2020-12/vocab/applicator

To request support for it, please open an issue at
https://github.com/intelligence-ai/jsonschema
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
