#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://json-schema.org/draft/2020-12/meta/format-annotation"
}
EOF

"$1" bundle "$TMP/schema.json" --http > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://json-schema.org/draft/2020-12/meta/format-annotation",
  "$defs": {
    "https://json-schema.org/draft/2020-12/meta/format-annotation": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://json-schema.org/draft/2020-12/meta/format-annotation",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/format-annotation": true
      },
      "$dynamicAnchor": "meta",
      "title": "Format vocabulary meta-schema for annotation results",
      "type": [
        "object",
        "boolean"
      ],
      "properties": {
        "format": {
          "type": "string"
        }
      }
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
