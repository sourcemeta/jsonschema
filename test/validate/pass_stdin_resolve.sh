#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/defs.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/defs",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

# Schema from stdin referencing a resolved external schema
cat << 'EOF' | "$1" validate - "$TMP/instance.json" --resolve "$TMP/defs.json" \
  --verbose > "$TMP/output.txt" 2>&1
{"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"https://example.com/defs"}
EOF

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance.json
  matches /dev/stdin
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
