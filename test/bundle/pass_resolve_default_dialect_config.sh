#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$ref": "nested"
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/remote.json" --verbose > "$TMP/result.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$id": "https://example.com",
  "$ref": "nested",
  "$defs": {
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/nested",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Detecting schema resources from file: $(realpath "$TMP")/remote.json
Importing schema into the resolution context: file://$(realpath "$TMP")/remote.json
Importing schema into the resolution context: https://example.com/nested
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
