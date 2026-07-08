#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "tags": {
      "type": "array",
      "x-jsonld-id": "https://schema.org/keywords",
      "items": { "type": "string" },
      "allOf": [
        { "x-jsonld-container": "@list" },
        { "x-jsonld-container": "@set" }
      ]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "tags": [ "a" ] }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: A JSON-LD container cannot be assigned more than one value
  at line 1
  at column 3
  at instance location "/tags"
  at facet "container"
  at file path $(realpath "$TMP")/instance.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "A JSON-LD container cannot be assigned more than one value",
  "line": 1,
  "column": 3,
  "instanceLocation": "/tags",
  "facet": "container",
  "filePath": "$(realpath "$TMP")/instance.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
