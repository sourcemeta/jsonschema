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
{ "foo": 1 }
{ "foo": 9223372036854776000 }
{ "foo": 3 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The JSON value is not representable by the IETF RFC 8259 interoperable signed integer range at line 2 and column 10
  $(realpath "$TMP")/instance.jsonl
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
