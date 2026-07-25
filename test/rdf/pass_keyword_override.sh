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
    "recordedAt": {
      "x-jsonld-id": "https://schema.org/dateCreated",
      "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#dateTimeStamp",
      "x-jsonld-override": true,
      "$ref": "#/$defs/library"
    }
  },
  "$defs": {
    "library": {
      "type": "string",
      "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#dateTime"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "recordedAt": "2026-01-15T10:00:00Z" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/dateCreated": [
      {
        "@value": "2026-01-15T10:00:00Z",
        "@type": "http://www.w3.org/2001/XMLSchema#dateTimeStamp"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
