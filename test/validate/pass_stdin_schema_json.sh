#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

cat << 'EOF' | "$1" validate - "$TMP/instance.json" --json > "$TMP/output.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "type": "object",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "valid": true,
  "annotations": [
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "https://example.com#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 16 ],
      "annotation": [ "foo" ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
