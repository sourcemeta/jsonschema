#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/foo.schema.json"
{
  "$id": "https://example.com/foo",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Foo",
  "description": "A foo schema",
  "examples": [ "bar" ],
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/foo.test.json"
{ "not": "a schema" }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "extension": [ ".schema.json" ]
}
EOF

cd "$TMP"
"$1" lint "$TMP/schemas" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using extension: .schema.json
Linting: $(realpath "$TMP")/schemas/foo.schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
