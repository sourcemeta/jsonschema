#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

cat << 'EOF' > "$TMP/no-id.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "boolean"
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/no-id.json" --verbose 2>"$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/no-id.json
Importing schema into the resolution context: file://$(realpath "$TMP")/no-id.json
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
