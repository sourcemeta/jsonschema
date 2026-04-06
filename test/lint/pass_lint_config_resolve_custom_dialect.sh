#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/v2"

cat << 'EOF' > "$TMP/v2/dialect.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/my-dialect",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/unevaluated": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/meta-data": true,
    "https://json-schema.org/draft/2020-12/vocab/format-annotation": true,
    "https://json-schema.org/draft/2020-12/vocab/content": true
  },
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/v2/person.json"
{
  "$schema": "https://example.com/my-dialect",
  "title": "Person",
  "description": "A person schema",
  "type": "object",
  "examples": [ { "name": "John" } ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/my-dialect": "./v2/dialect.json"
  }
}
EOF

BIN="$(realpath "$1")"
cd "$TMP"
"$BIN" lint v2/person.json --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Linting: $(realpath "$TMP")/v2/person.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
