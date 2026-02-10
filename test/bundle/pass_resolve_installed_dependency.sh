#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project/vendor"

cat << 'EOF' > "$TMP/source/types.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/types",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/types.json": "./vendor/types.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

# Now create a schema that references the installed dependency
cat << 'EOF' > "$TMP/project/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/types"
}
EOF

# Bundle should pick up the installed dependency automatically
"$1" bundle "$TMP/project/schema.json" > "$TMP/output.json" 2>/dev/null

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/types",
  "$defs": {
    "https://example.com/types": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/types",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
