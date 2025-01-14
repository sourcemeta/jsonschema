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
  "$ref": "nested"
}
EOF

mkdir "$TMP/schemas"
cat << 'EOF' > "$TMP/schemas/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/schemas" --without-id --verbose 1> "$TMP/result.json" 2>&1

cat << EOF > "$TMP/expected.json"
Importing schema into the resolution context: $(realpath "$TMP")/schemas/remote.json
Removing schema identifiers
Importing schema into the resolution context: $(realpath "$TMP")/schemas/remote.json
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$ref": "#/\$defs/https%3A~1~1example.com~1nested",
  "\$defs": {
    "https://example.com/nested": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "type": "string"
    }
  }
}
EOF

cat "$TMP/result.json"

diff "$TMP/result.json" "$TMP/expected.json"
