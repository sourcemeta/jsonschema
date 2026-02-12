#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/user.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/source/product.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "number"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/product.json": "./vendor/product.json",
    "file://$(realpath "$TMP")/source/user.json": "./vendor/user.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/product.json
Installed      : $(realpath "$TMP")/project/vendor/product.json
Fetching       : file://$(realpath "$TMP")/source/user.json
Installed      : $(realpath "$TMP")/project/vendor/user.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" ci > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Up to date     : file://$(realpath "$TMP")/source/product.json
Up to date     : file://$(realpath "$TMP")/source/user.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
