#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": "string",
  "$defs": {
    "foo": {
      "type": [ "string", "null" ]
    }
  }
}
EOF

"$1" metaschema --default-dialect "https://json-schema.org/draft/2020-12/schema" \
  "$TMP/schema.json"
