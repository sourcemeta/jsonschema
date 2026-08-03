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
  "x-jsonld-type": "https://schema.org/Product",
  "x-jsonld-constants": {
    "https://schema.org/brand": null
  },
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Vacuum Robot" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: A JSON-LD constants entry can only be null inside an overriding schema object
  at line 1
  at column 1
  at instance location ""
  at facet "constants"
  at schema location file://$(realpath "$TMP")/schema.json#/x-jsonld-constants
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
  "error": "A JSON-LD constants entry can only be null inside an overriding schema object",
  "line": 1,
  "column": 1,
  "instanceLocation": "",
  "facet": "constants",
  "schemaLocation": "file://$(realpath "$TMP")/schema.json#/x-jsonld-constants",
  "filePath": "$(realpath "$TMP")/instance.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
