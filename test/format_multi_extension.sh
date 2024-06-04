#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema_1.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/schema_2.schema.json"
{"type": "string", "title": "My String"}
EOF

cd "$TMP"
"$1" fmt --extension .schema.json -v

cat << 'EOF' > "$TMP/expected_1.json"
{
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/expected_2.json"
{
  "title": "My String",
  "type": "string"
}
EOF

diff "$TMP/schema_1.json" "$TMP/expected_1.json"
diff "$TMP/schema_2.schema.json" "$TMP/expected_2.json"
