#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "x": {
      "allOf": [
        { "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#date" },
        { "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#string" }
      ]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Conflicting datatypes",
      "valid": true,
      "data": { "x": "v" },
      "rdf": []
    },
    {
      "description": "No conflicting member",
      "valid": true,
      "data": {},
      "rdf": []
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  1/2 FAIL Conflicting datatypes

error: A JSON-LD datatype cannot be assigned more than one value
  at line 7
  at column 17
  at instance location "/x"
  at facet "datatype"
  at schema location https://example.com#/properties/x/allOf/0/x-jsonld-datatype
  at conflicting schema location https://example.com#/properties/x/allOf/1/x-jsonld-datatype
  at file path $(realpath "$TMP")/test.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
