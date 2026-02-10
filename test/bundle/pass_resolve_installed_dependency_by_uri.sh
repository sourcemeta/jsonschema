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
"$1" install > "$TMP/install_output.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/types.json
Installed      : $(realpath "$TMP")/project/vendor/types.json
EOF

diff "$TMP/install_output.txt" "$TMP/expected_install.txt"

cat << EOF > "$TMP/project/schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/main",
  "\$ref": "file://$(realpath "$TMP")/source/types.json"
}
EOF

"$1" bundle "$TMP/project/schema.json" > "$TMP/output.json" 2>/dev/null

cat << EOF > "$TMP/expected.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/main",
  "\$ref": "file://$(realpath "$TMP")/source/types.json",
  "\$defs": {
    "file://$(realpath "$TMP")/source/types.json": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "file://$(realpath "$TMP")/source/types.json",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
