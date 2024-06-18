#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
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
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The target document is expected to be of the given type
  at instance location "/foo"
  at evaluate path "/properties/foo/type"
error: The target is expected to match all of the given assertions
  at instance location ""
  at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
