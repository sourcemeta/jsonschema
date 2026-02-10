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
"$1" install --debug > "$TMP/output.txt" 2>&1

# Debug output should contain debug-prefixed lines
grep -q "^debug: fetch/start:" "$TMP/output.txt"
grep -q "^debug: fetch/end:" "$TMP/output.txt"
grep -q "^debug: bundle/start:" "$TMP/output.txt"
grep -q "^debug: bundle/end:" "$TMP/output.txt"
grep -q "^debug: write/start:" "$TMP/output.txt"
grep -q "^debug: write/end:" "$TMP/output.txt"
grep -q "^debug: verify/start:" "$TMP/output.txt"
grep -q "^debug: verify/end:" "$TMP/output.txt"

# Normal output should also be present
grep -q "^Fetching" "$TMP/output.txt"
grep -q "^Installed" "$TMP/output.txt"

test -f "$TMP/project/vendor/user.json"
test -f "$TMP/project/jsonschema.lock.json"
