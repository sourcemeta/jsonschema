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
"$1" install > /dev/null 2>&1

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

rm "$TMP/project/vendor/schema.json"

"$1" install --frozen --debug > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
debug: fetch/start: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
Fetching       : file://$(realpath "$TMP")/source/schema.json
debug: Attempting to read file reference from disk: $(realpath "$TMP")/source/schema.json
debug: fetch/end: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
debug: bundle/start: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
Bundling       : file://$(realpath "$TMP")/source/schema.json
debug: bundle/end: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
debug: write/start: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
Writing        : $(realpath "$TMP")/project/vendor/schema.json
debug: write/end: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
debug: verify/start: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
Verifying      : $(realpath "$TMP")/project/vendor/schema.json
debug: verify/end: file://$(realpath "$TMP")/source/schema.json (1/1) -> $(realpath "$TMP")/project/vendor/schema.json
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
