#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "top_level_description",
      "message": "Set a non-empty description at the top level of the schema to explain what the definition is about in detail",
      "description": null,
      "schemaLocation": "",
      "position": [ 1, 1, 4, 1 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "top_level_title",
      "message": "Set a concise non-empty title at the top level of the schema to explain what the definition is about",
      "description": null,
      "schemaLocation": "",
      "position": [ 1, 1, 4, 1 ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
