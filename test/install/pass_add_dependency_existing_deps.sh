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
  "type": "object"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/user.json": "./vendor/user.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/setup_output.txt" 2>&1

cat << EOF > "$TMP/expected_setup.txt"
Fetching       : file://$(realpath "$TMP")/source/user.json
Installed      : $(realpath "$TMP")/project/vendor/user.json
EOF

diff "$TMP/setup_output.txt" "$TMP/expected_setup.txt"

"$1" install "file://$(realpath "$TMP")/source/product.json" "./vendor/product.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Adding         : file://$(realpath "$TMP")/source/product.json -> ./vendor/product.json
Fetching       : file://$(realpath "$TMP")/source/product.json
Installed      : $(realpath "$TMP")/project/vendor/product.json
Up to date     : file://$(realpath "$TMP")/source/user.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_config.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/product.json": "./vendor/product.json",
    "file://$(realpath "$TMP")/source/user.json": "./vendor/user.json"
  }
}
EOF

diff "$TMP/project/jsonschema.json" "$TMP/expected_config.json"

cat << EOF > "$TMP/expected_product.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "\$id": "file://$(realpath "$TMP")/source/product.json"
}
EOF

diff "$TMP/project/vendor/product.json" "$TMP/expected_product.json"

HASH_PRODUCT="$(shasum -a 256 < "$TMP/project/vendor/product.json" | cut -d ' ' -f 1)"
HASH_USER="$(shasum -a 256 < "$TMP/project/vendor/user.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/product.json": {
      "path": "$(realpath "$TMP")/project/vendor/product.json",
      "hash": "${HASH_PRODUCT}",
      "hashAlgorithm": "sha256"
    },
    "file://$(realpath "$TMP")/source/user.json": {
      "path": "$(realpath "$TMP")/project/vendor/user.json",
      "hash": "${HASH_USER}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
