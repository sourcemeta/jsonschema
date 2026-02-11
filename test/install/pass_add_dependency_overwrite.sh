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
    "file://$(realpath "$TMP")/source/user.json": "./vendor/old_path.json"
  }
}
EOF

cd "$TMP/project"
"$1" install "file://$(realpath "$TMP")/source/user.json" "./vendor/new_path.json" \
  > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Adding         : file://$(realpath "$TMP")/source/user.json -> ./vendor/new_path.json
Fetching       : file://$(realpath "$TMP")/source/user.json
Installed      : $(realpath "$TMP")/project/vendor/new_path.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_config.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/user.json": "./vendor/new_path.json"
  }
}
EOF

diff "$TMP/project/jsonschema.json" "$TMP/expected_config.json"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "\$id": "file://$(realpath "$TMP")/source/user.json"
}
EOF

diff "$TMP/project/vendor/new_path.json" "$TMP/expected_schema.json"

test ! -f "$TMP/project/vendor/old_path.json"
