#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
[
  {
    "target": "https://json-schema.org/draft/2020-12/schema",
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
]
EOF

"$1" test "$TMP/test.json" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: The test document must be an object

Learn more here: https://github.com/Intelligence-AI/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
