#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/alpha.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/source/beta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "integer"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/alpha.json": "./vendor/alpha.json",
    "file://$(realpath "$TMP")/source/beta.json": "./vendor/beta.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/alpha.json
Installed      : $(realpath "$TMP")/project/vendor/alpha.json
Fetching       : file://$(realpath "$TMP")/source/beta.json
Installed      : $(realpath "$TMP")/project/vendor/beta.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

test -f "$TMP/project/vendor/alpha.json"
test -f "$TMP/project/vendor/beta.json"
test -f "$TMP/project/jsonschema.lock.json"
