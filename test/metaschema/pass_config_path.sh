#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo"
mkdir -p "$TMP/bar"

cat << 'EOF' > "$TMP/foo/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./foo"
}
EOF

cd "$TMP/bar"
"$1" metaschema --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
ok: $(realpath "$TMP")/foo/schema.json
  matches http://json-schema.org/draft-04/schema#
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
