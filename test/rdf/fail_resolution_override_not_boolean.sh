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
    "name": {
      "type": "string",
      "x-jsonld-id": "https://schema.org/name",
      "x-jsonld-override": "yes"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: The value of x-jsonld-override must be a boolean
  at line 1
  at column 3
  at instance location "/name"
  at facet "override"
  at schema location file://$(realpath "$TMP")/schema.json#/properties/name/x-jsonld-override
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
  "error": "The value of x-jsonld-override must be a boolean",
  "line": 1,
  "column": 3,
  "instanceLocation": "/name",
  "facet": "override",
  "schemaLocation": "file://$(realpath "$TMP")/schema.json#/properties/name/x-jsonld-override",
  "filePath": "$(realpath "$TMP")/instance.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
