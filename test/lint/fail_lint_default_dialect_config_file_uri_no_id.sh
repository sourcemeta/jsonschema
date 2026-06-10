#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/project/schemas" "$TMP/meta"

cat << 'EOF' > "$TMP/meta/custom.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/meta-data": true
  },
  "$dynamicAnchor": "meta"
}
EOF

cat << 'EOF' > "$TMP/project/schemas/sample.json"
{ "type": "string" }
EOF

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "path": "./schemas",
  "defaultDialect": "../meta/custom"
}
EOF

cd "$TMP/project"

"$1" lint > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
schemas/sample.json:1:1:
  Set a concise non-empty title at the top level of the schema to explain what the definition is about (top_level_title)
    at location ""
schemas/sample.json:1:1:
  Set a non-empty description at the top level of the schema to explain what the definition is about in detail (top_level_description)
    at location ""
schemas/sample.json:1:1:
  Set a non-empty examples array at the top level of the schema to illustrate the expected data (top_level_examples)
    at location ""
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
