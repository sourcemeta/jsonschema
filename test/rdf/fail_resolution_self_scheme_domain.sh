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
    "email": {
      "type": "string",
      "x-jsonld-id": "https://schema.org/email",
      "x-jsonld-self": "mailto"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "email": "not-an-email" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: A JSON-LD self identity value is outside the domain of its scheme
  at line 1
  at column 3
  at instance location "/email"
  at facet "self"
  at schema location file://$(realpath "$TMP")/schema.json#/properties/email/x-jsonld-self
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
  "error": "A JSON-LD self identity value is outside the domain of its scheme",
  "line": 1,
  "column": 3,
  "instanceLocation": "/email",
  "facet": "self",
  "schemaLocation": "file://$(realpath "$TMP")/schema.json#/properties/email/x-jsonld-self",
  "filePath": "$(realpath "$TMP")/instance.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
