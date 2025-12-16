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
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    },
    "age": {
      "type": "integer"
    }
  }
}
EOF

mkdir -p "$TMP/instances/nested"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice", "age": 30 }
EOF

cat << 'EOF' > "$TMP/instances/nested/instance_2.json"
{ "name": "Bob", "age": "invalid" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --verbose 2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instances/instance_1.json
  matches $(realpath "$TMP")/schema.json
fail: $(realpath "$TMP")/instances/nested/instance_2.json
error: Schema validation failure
  The value was expected to be of type integer but it was of type string
    at instance location "/age" (line 1, column 18)
    at evaluate path "/properties/age/type"
  The object value was expected to validate against the defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
