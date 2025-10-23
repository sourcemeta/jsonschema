#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance.jsonl"
{ "foo": "first" }
{ "foo"  "second" }
{ "foo": "third" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 2
  at column 10
  at file path $(realpath "$TMP")/instance.jsonl
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
