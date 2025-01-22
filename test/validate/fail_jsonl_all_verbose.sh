#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "array"
}
EOF

cat << 'EOF' > "$TMP/instance.jsonl"
{ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl" --verbose 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
Interpreting input as JSONL: $(realpath "$TMP")/instance.jsonl
fail: $(realpath "$TMP")/instance.jsonl (entry #1)

{
  "foo": 1
}

error: Schema validation failure
  The value was expected to be of type array but it was of type object
    at instance location ""
    at evaluate path "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
