#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{
  "components": {
    "schemas": {
      "Address": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object"
      },
      "User": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$ref": "#/components/schemas/Address"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/User" \
  > /dev/null 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation should fail when $ref points outside extracted subtree
test "$EXIT_CODE" -ne "0"

