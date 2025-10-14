#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/output.json"

cat << EOF > "$TMP/expected.json"
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

diff "$TMP/expected.json" "$TMP/output.json"
