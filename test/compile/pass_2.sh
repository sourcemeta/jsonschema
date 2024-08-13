#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "allOf": [ {"$ref": "#/definitions/job" } ],
  "definitions": { "job": {} }
}
EOF

# We cannot assert on the result as the reference generates a random label
"$1" compile "$TMP/schema.json"
