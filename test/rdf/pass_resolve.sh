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
    "person": { "$ref": "https://example.com/person" }
  }
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$id": "https://example.com/person",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-id": "https://schema.org/author",
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "person": { "name": "Ada" } }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/remote.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/author": [
      {
        "@type": [ "https://schema.org/Person" ],
        "https://schema.org/name": [
          {
            "@value": "Ada"
          }
        ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
