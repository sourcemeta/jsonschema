#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
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

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" --verbose 2> "$TMP/output.txt" 1>&2

cat << EOF > "$TMP/expected.txt"
Interpreting input as JSONL
ok: $(realpath "$TMP")/instance.jsonl (entry #0)
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/instance.jsonl (entry #1)
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/instance.jsonl (entry #2)
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
