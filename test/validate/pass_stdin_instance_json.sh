#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

echo '{ "foo": "bar" }' | "$1" validate "$TMP/schema.json" - --json > "$TMP/output.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "valid": true,
  "annotations": [
    {
      "keywordLocation": "/description",
      "absoluteKeywordLocation": "https://example.com#/description",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 16 ],
      "annotation": [ "Test schema" ]
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "https://example.com#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 16 ],
      "annotation": [ "foo" ]
    },
    {
      "keywordLocation": "/title",
      "absoluteKeywordLocation": "https://example.com#/title",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 16 ],
      "annotation": [ "Test" ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
