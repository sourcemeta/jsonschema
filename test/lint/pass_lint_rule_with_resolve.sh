#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/defs.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/defs",
  "$defs": {
    "has_type": {
      "required": [ "type" ]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/rule.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "require_type",
  "description": "Every subschema must declare the type keyword",
  "$ref": "https://example.com/defs#/$defs/has_type"
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"
"$1" lint --rule "$TMP/rule.json" --resolve "$TMP/defs.json" \
  --only require_type "$TMP/schema.json" \
  >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
schema.json:1:1:
  Every subschema must declare the type keyword (require_type)
    at location ""
    The object value was expected to define the property "type"
      at instance location ""
      at evaluate path "/$ref/required"
    The object value was expected to validate against the referenced schema
      at instance location ""
      at evaluate path "/$ref"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
