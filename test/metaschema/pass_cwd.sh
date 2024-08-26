#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema_1.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schema_2.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "object"
}
EOF

cd "$TMP"
"$1" metaschema --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/schema_1.json
  matches http://json-schema.org/draft-04/schema#
ok: $(realpath "$TMP")/schema_2.json
  matches http://json-schema.org/draft-04/schema#
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
