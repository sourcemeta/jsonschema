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
    "age": { "type": "integer" },
    "status": { "enum": ["active", "inactive"] },
    "name": { "type": "string", "maxLength": 50 }
  },
  "required": ["status"]
}
EOF

cat << 'EOF' > "$TMP/new.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "age": { "type": "integer" },
    "status": { "enum": ["active"] },
    "name": { "type": "string", "maxLength": 30 },
    "nickname": { "type": "string" }
  },
  "required": ["status", "age"]
}
EOF

"$1" compat "$TMP/old.json" "$TMP/new.json" > "$TMP/stdout" 2> "$TMP/stderr"

if [ -s "$TMP/stderr" ]
then
  echo "FAIL: Unexpected stderr output for compat command" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.txt"
Breaking Changes:

- property "age" is now required
- enum "status" removed value "inactive"

Warnings:

- maxLength reduced from 50 to 30 for property "name"

Safe Changes:

- added optional property "nickname"
EOF

diff "$TMP/stdout" "$TMP/expected.txt"

echo "PASS" 1>&2
