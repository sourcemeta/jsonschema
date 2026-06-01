#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/metaschema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/custom-metaschema",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true,
    "https://json-schema.org/draft/2020-12/vocab/meta-data": true,
    "https://json-schema.org/draft/2020-12/vocab/format-assertion": true
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "title": "Email",
  "description": "An email address",
  "type": "string",
  "format": "email",
  "examples": [ "foo@bar.com" ]
}
EOF

"$1" lint "$TMP/schema.json" \
  --resolve "$TMP/metaschema.json" > "$TMP/stderr.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
