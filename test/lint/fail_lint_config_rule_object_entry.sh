#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_id",
  "description": "The root schema must declare an $id",
  "required": [ "$id" ]
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "rules": [
      { "path": "./rule.json" }
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/schema",
  "type": "object",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

cd "$TMP"
"$1" lint --only require_id "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
schema.json:6:5:
  The root schema must declare an $id (require_id)
    at location "/properties/foo"
    The object value was expected to define the property "$id"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cd "$TMP"
"$1" lint --only require_id --json "$TMP/schema.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected_json.txt"
{
  "valid": false,
  "health": 50,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "require_id",
      "message": "The root schema must declare an \$id",
      "description": "The object value was expected to define the property \"\$id\"",
      "schemaLocation": [ "properties", "foo" ],
      "position": [ 6, 5, 6, 31 ]
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
