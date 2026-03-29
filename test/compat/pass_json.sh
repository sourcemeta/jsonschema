#!/bin/sh
set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/old.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "value": { "type": "integer" }
  }
}
EOF

cat << 'EOF' > "$TMP/new.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "value": { "type": ["integer", "string"] }
  }
}
EOF

"$1" compat "$TMP/old.json" "$TMP/new.json" --json > "$TMP/stdout" 2> "$TMP/stderr"

if [ -s "$TMP/stderr" ]
then
  echo "FAIL: Unexpected stderr output for compat JSON output" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.txt"
{
  "breaking": [],
  "warnings": [],
  "safe": [
    {
      "kind": "type_widening",
      "path": "value",
      "message": "property \"value\" widened type from \"integer\" to [\"integer\", \"string\"]"
    }
  ],
  "summary": {
    "breaking": 0,
    "warnings": 0,
    "safe": 1,
    "compatible": true
  }
}
EOF

diff "$TMP/stdout" "$TMP/expected.txt"

echo "PASS" 1>&2
