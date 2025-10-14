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
  --verbose 2> "$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance_1.json
  matches $(realpath "$TMP")/schema.json
fail: $(realpath "$TMP")/instance_2.json
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "/foo" (line 1, column 3)
    at evaluate path "/properties/foo/type"
  The object value was expected to validate against the single defined property subschema
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
ok: $(realpath "$TMP")/instance_3.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
