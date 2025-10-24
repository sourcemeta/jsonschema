#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema_1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": false,
  "title": "Hello World",
  "properties": {"foo": {}, "bar": {}}
}
EOF

cat << 'EOF' > "$TMP/schema_2.json"
{"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "string", "title": "My String"}
EOF

"$1" fmt "$TMP"

cat << 'EOF' > "$TMP/expected_1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Hello World",
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
  "type": "string"
}
EOF

diff "$TMP/schema_1.json" "$TMP/expected_1.json"
diff "$TMP/schema_2.json" "$TMP/expected_2.json"
