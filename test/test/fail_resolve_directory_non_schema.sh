#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "definitions": {
    "foo": { "type": "string" },
    "bar": { "type": "integer" }
  }
}
EOF

cat << 'EOF' > "$TMP/schemas/test.json"
{
  "target": "https://example.com#/definitions/foo",
  "tests": [
    {
      "description": "Fail",
      "valid": true,
      "data": 5
    }
  ]
}
EOF

"$1" test "$TMP/schemas/test.json" --verbose --resolve "$TMP" 1> "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Importing schema into the resolution context: $(realpath "$TMP")/schemas/schema.json
Importing schema into the resolution context: $(realpath "$TMP")/schemas/test.json
error: Cannot determine the base dialect of the schema
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
