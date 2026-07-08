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
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "$ref": "./schemas/name.json" }
  }
}
EOF

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/name.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "Ada" }
EOF

"$1" rdf "$TMP/schema.json" --resolve "$TMP/schemas" "$TMP/instance.json" \
  > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "@type": [ "https://schema.org/Person" ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
