#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/project"
mkdir -p "$TMP/other"

cat << 'EOF' > "$TMP/project/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "lint": {
    "rules": [
      "./rule.json"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/project/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP/other"
"$1" lint --only require_type "$TMP/project/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
../project/schema.json:1:1:
  Every subschema must declare the type keyword (require_type)
    at location ""
    The object value was expected to define the property "type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP/other"
"$1" lint --only require_type --json "$TMP/project/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/project/schema.json",
      "id": "require_type",
      "message": "Every subschema must declare the type keyword",
      "description": "The object value was expected to define the property \"type\"",
      "schemaLocation": "",
      "position": [ 1, 1, 3, 1 ]
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
