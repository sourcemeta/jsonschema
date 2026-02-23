#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule_type.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "required": [ "type" ]
}
EOF

cat << 'EOF' > "$TMP/rule_description.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_description",
  "description": "Every subschema must declare the description keyword",
  "required": [ "description" ]
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" lint --rule "$TMP/rule_type.json" --rule "$TMP/rule_description.json" \
  --only require_type --only require_description --json "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "require_description",
      "message": "Every subschema must declare the description keyword",
      "description": "The object value was expected to define the property \"description\"",
      "schemaLocation": "",
      "position": [ 1, 1, 4, 1 ]
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
