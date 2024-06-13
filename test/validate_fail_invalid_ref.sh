#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "properties": {
    "foo": {
      "$ref": "#/definitions/i-dont-exist"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2>"$TMP/stderr.txt" \
  && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.txt"
Could not resolve schema reference: #/definitions/i-dont-exist
    at schema location "/properties/foo/$ref"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
echo "PASS" 1>&2
