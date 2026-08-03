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
    "spec": {
      "type": "object",
      "x-jsonld-id": "https://schema.org/weight",
      "x-jsonld-value": "https://schema.org/value"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "spec": { "a": 1 } }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: A JSON-LD value predicate can only be assigned to a scalar value
  at line 1
  at column 3
  at instance location "/spec"
  at facet "value"
  at schema location file://$(realpath "$TMP")/schema.json#/properties/spec/x-jsonld-value
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
  "error": "A JSON-LD value predicate can only be assigned to a scalar value",
  "line": 1,
  "column": 3,
  "instanceLocation": "/spec",
  "facet": "value",
  "schemaLocation": "file://$(realpath "$TMP")/schema.json#/properties/spec/x-jsonld-value",
  "filePath": "$(realpath "$TMP")/instance.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
