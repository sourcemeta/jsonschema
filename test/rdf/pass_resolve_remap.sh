#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "person": { "$ref": "other" }
  }
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$id": "https://example.com/nested",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-id": "https://schema.org/author",
  "x-jsonld-type": "https://schema.org/Person"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/other": "https://example.com/nested"
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "person": {} }
EOF

cd "$TMP"

"$1" rdf schema.json instance.json --resolve remote.json > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/author": [
      {
        "@type": [ "https://schema.org/Person" ]
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
