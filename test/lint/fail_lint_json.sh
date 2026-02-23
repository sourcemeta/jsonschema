#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "enum_with_type",
      "message": "Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": "/type",
      "position": [ 5, 3, 5, 18 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "enum_with_type",
      "message": "Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": "/enum",
      "position": [ 6, 3, 6, 19 ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
