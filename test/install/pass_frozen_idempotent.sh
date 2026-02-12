#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" install --frozen > "$TMP/output_first.txt" 2>&1

cat << EOF > "$TMP/expected_first.txt"
Up to date     : file://$(realpath "$TMP")/source/schema.json
EOF

diff "$TMP/output_first.txt" "$TMP/expected_first.txt"

"$1" install --frozen > "$TMP/output_second.txt" 2>&1

diff "$TMP/output_second.txt" "$TMP/expected_first.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
