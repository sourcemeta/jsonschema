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
    "https://json-schema.org/draft/2020-12/vocab/format-assertion": false
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/custom-metaschema",
  "type": "object",
  "properties": {
    "email": {
      "type": "string",
      "format": "email"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "email": "not-an-email" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/metaschema.json" --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
annotation: "email"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
annotation: "email"
  at instance location "/email" (line 1, column 3)
  at evaluate path "/properties/email/format"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
