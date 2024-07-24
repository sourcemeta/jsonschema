#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance_1.json"
{ "foo": "bar" }
EOF

cat << 'EOF' > "$TMP/instance_2.json"
{ "foo": 1 }
EOF

cat << 'EOF' > "$TMP/instance_3.json"
{ "foo": "baz" }
EOF

"$1" validate "$TMP/schema.json" \
  "$TMP/instance_1.json" \
  "$TMP/instance_2.json" \
  "$TMP/instance_3.json" \
  2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance_2.json
error: Schema validation failure
  The target document is expected to be of the given type
    at instance location "/foo"
    at evaluate path "/properties/foo/type"
  The target is expected to match all of the given assertions
    at instance location ""
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
