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
  "type": "array"
}
EOF

cat << 'EOF' | gzip > "$TMP/instance.jsonl.gz"
{ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.jsonl.gz" --continue 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.jsonl.gz (entry #1)

{
  "foo": 1
}

error: Schema validation failure
  The value was expected to be of type array but it was of type object
    at instance location ""
    at evaluate path "/type"

fail: $(realpath "$TMP")/instance.jsonl.gz (entry #2)

{
  "foo": 2
}

error: Schema validation failure
  The value was expected to be of type array but it was of type object
    at instance location ""
    at evaluate path "/type"

fail: $(realpath "$TMP")/instance.jsonl.gz (entry #3)

{
  "foo": 3
}

error: Schema validation failure
  The value was expected to be of type array but it was of type object
    at instance location ""
    at evaluate path "/type"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
