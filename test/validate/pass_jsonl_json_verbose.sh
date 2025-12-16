#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.jsonl"
{ "foo": "first" }
{ "foo": "second" }
{ "foo": "third" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" --json --verbose > "$TMP/output.json" 2>&1

cat << EOF > "$TMP/expected.json"
Interpreting input as JSONL: $(realpath "$TMP")/instance.jsonl
{
  "valid": true
}
{
  "valid": true
}
{
  "valid": true
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
