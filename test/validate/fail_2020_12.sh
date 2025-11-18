#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "/foo" (line 1, column 3)
    at evaluate path "/properties/foo/type"
  The object value was expected to validate against the single defined property subschema
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
