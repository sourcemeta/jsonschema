#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/main.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/types/name"
}
EOF

cat << 'EOF' > "$TMP/project/name.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/types/name",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "resolve": {
    "https://example.com/types/name": "./name.json"
  },
  "dependencies": {
    "file://$(realpath "$TMP")/source/main.json": "./vendor/main.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/main.json
Installed      : $(realpath "$TMP")/project/vendor/main.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

rm "$TMP/project/vendor/main.json"

"$1" install --frozen > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/main.json
Installed      : $(realpath "$TMP")/project/vendor/main.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/main",
  "\$ref": "https://example.com/types/name",
  "\$defs": {
    "https://example.com/types/name": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "https://example.com/types/name",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/project/vendor/main.json" "$TMP/expected_schema.json"
