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

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" metaschema --verbose "$TMP/schema.json" 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/schema.json
  matches https://json-schema.org/draft/2020-12/schema
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
