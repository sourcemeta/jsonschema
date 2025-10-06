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

cat << 'EOF' > "$TMP/schema_2.json"
{"type": "string", "title": "My String"}
EOF

"$1" fmt "$TMP" --ignore "$TMP/schema_2.json"

cat << 'EOF' > "$TMP/expected_1.json"
{
  "title": "Hello World",
  "properties": {
    "bar": {},
    "foo": {}
  },
  "additionalProperties": false
}
EOF

cat << 'EOF' > "$TMP/expected_2.json"
{"type": "string", "title": "My String"}
EOF

diff "$TMP/schema_1.json" "$TMP/expected_1.json"
diff "$TMP/schema_2.json" "$TMP/expected_2.json"
