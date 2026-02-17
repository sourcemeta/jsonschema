#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "all_properties_camelcase",
  "description": "Ensures camelCase properties",
  "properties": {
    "properties": {
      "propertyNames": {
        "pattern": "^[a-z][a-zA-Z0-9]*$"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Example Schema",
  "description": "Contains a non-camel-case property",
  "examples": [ { "INVALID_PROPERTY": "foo" } ],
  "properties": {
    "INVALID_PROPERTY": {
      "type": "string"
    }
  }
}
EOF

cd "$TMP"
"$1" lint --rule "$TMP/rule.json" "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
schema.json:7:5:
  Ensures camelCase properties (all_properties_camelcase)
    at location "/properties/INVALID_PROPERTY"
    The property name "INVALID_PROPERTY" was expected to match the regular expression "^[a-z][a-zA-Z0-9]*$"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
