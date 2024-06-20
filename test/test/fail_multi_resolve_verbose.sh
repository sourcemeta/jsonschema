#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "schema": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "First failure",
      "valid": true,
      "data": 1
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    },
    {
      "description": "Second failure",
      "valid": false,
      "data": "foo"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Importing schema into the resolution context: $(realpath "$TMP")/schema.json
$(realpath "$TMP")/test.json:
  1/4 PASS First test
  2/4 FAIL First failure

error: The target document is expected to be of the given type
  at instance location ""
  at evaluate path "/type"

  3/4 PASS Invalid type
  4/4 FAIL Second failure

error: passed but was expected to fail
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
