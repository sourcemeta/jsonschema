#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/proj1/schemas"
mkdir -p "$TMP/proj2/schemas"

cat << 'EOF' > "$TMP/proj1/jsonschema.json"
{
  "extension": [ ".schema.json" ]
}
EOF

cat << 'EOF' > "$TMP/proj2/jsonschema.json"
{
  "extension": [ ".spec.json" ]
}
EOF

cat << 'EOF' > "$TMP/proj1/schemas/foo.schema.json"
{
  "$id": "https://example.com/foo",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Foo",
  "description": "A foo schema",
  "examples": [ "bar" ],
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/proj1/schemas/foo.test.json"
{ "not": "a schema" }
EOF

cat << 'EOF' > "$TMP/proj2/schemas/bar.spec.json"
{
  "$id": "https://example.com/bar",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Bar",
  "description": "A bar schema",
  "examples": [ 1 ],
  "type": "integer"
}
EOF

cat << 'EOF' > "$TMP/proj2/schemas/bar.test.json"
{ "not": "a schema" }
EOF

cd "$TMP"
"$1" lint "$TMP/proj1/schemas" "$TMP/proj2/schemas" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using extension: .schema.json
Using extension: .spec.json
Linting: $(realpath "$TMP")/proj1/schemas/foo.schema.json
Linting: $(realpath "$TMP")/proj2/schemas/bar.spec.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
