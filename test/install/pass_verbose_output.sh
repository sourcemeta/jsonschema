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

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/user.json": "./vendor/user.json"
  }
}
EOF

cd "$TMP/project"
"$1" install --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/user.json
Bundling       : file://$(realpath "$TMP")/source/user.json
Writing        : $(realpath "$TMP")/project/vendor/user.json
Verifying      : $(realpath "$TMP")/project/vendor/user.json
Installed      : $(realpath "$TMP")/project/vendor/user.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
