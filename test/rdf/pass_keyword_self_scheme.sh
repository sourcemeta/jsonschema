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
      "email": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/email",
        "x-jsonld-self": "mailto"
      },
      "account": {
        "type": "string",
        "x-jsonld-id": "https://schema.org/identifier",
        "x-jsonld-self": "acct"
      }
    }
  }
EOF

cat << 'EOF' > "$TMP/instance.json"
{
  "email": "gorby%kremvax@EXAMPLE.com",
  "account": "juliet@capulet.example@Shoppingsite.Example"
}
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/identifier": [
      {
        "@id": "acct:juliet%40capulet.example@shoppingsite.example"
      }
    ],
    "https://schema.org/email": [
      {
        "@id": "mailto:gorby%25kremvax@example.com"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
