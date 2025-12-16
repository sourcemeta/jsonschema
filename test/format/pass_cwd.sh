#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema_1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "Test schema",
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/schema_2.json"
{"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "string", "title": "My String", "description": "Test schema"}
EOF

cd "$TMP"
"$1" fmt >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected_1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Hello World",
  "description": "Test schema",
  "properties": {
    "foo": {},
    "bar": {}
  },
  "additionalProperties": false
}
EOF

cat << 'EOF' > "$TMP/expected_2.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "My String",
  "description": "Test schema",
  "type": "string"
}
EOF

diff "$TMP/schema_1.json" "$TMP/expected_1.json"
diff "$TMP/schema_2.json" "$TMP/expected_2.json"
