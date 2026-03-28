#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
{
  "components": {
    "schemas": {
      "User": [ "not", "a", "schema" ]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/document.json" "$TMP/instance.json" \
  --path "/components/schemas/User" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: The schema file you provided does not represent a valid JSON Schema
  at file path $(realpath "$TMP")/document.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
