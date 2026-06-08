#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/my-metaschema",
  "title": "My Custom Meta-Schema",
  "type": [ "object", "boolean" ],
  "properties": {
    "type": { "type": "string" }
  }
}
EOF

"$1" upgrade --meta --to 2019-09 "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/my-metaschema",
  "$vocabulary": {
    "https://json-schema.org/draft/2019-09/vocab/core": true,
    "https://json-schema.org/draft/2019-09/vocab/applicator": true,
    "https://json-schema.org/draft/2019-09/vocab/validation": true,
    "https://json-schema.org/draft/2019-09/vocab/meta-data": true,
    "https://json-schema.org/draft/2019-09/vocab/format": false,
    "https://json-schema.org/draft/2019-09/vocab/content": true
  },
  "title": "My Custom Meta-Schema",
  "type": [ "object", "boolean" ],
  "properties": {
    "type": {
      "type": "string"
    }
  }
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
