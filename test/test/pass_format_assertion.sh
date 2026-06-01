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
    "https://json-schema.org/draft/2020-12/vocab/format-assertion": true
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "$id": "https://example.com/my-schema",
  "type": "string",
  "format": "email"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com/my-schema",
  "tests": [
    {
      "description": "A valid instance",
      "valid": true,
      "data": "foo@bar.com"
    },
    {
      "description": "An invalid instance",
      "valid": false,
      "data": "not-an-email"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" \
  --resolve "$TMP/metaschema.json" --resolve "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json: PASS 2/2
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
