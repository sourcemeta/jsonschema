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
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/deep/nested/schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output_install.txt" 2>&1

cat << EOF > "$TMP/expected_install.txt"
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/deep/nested/schema.json
EOF

diff "$TMP/output_install.txt" "$TMP/expected_install.txt"

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

rm -rf "$TMP/project/vendor"

"$1" ci > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/deep/nested/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "\$id": "file://$(realpath "$TMP")/source/schema.json"
}
EOF

diff "$TMP/project/vendor/deep/nested/schema.json" "$TMP/expected_schema.json"
