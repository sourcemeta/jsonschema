#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance1.json"
{ "a": 1 }
EOF

cat << 'EOF' > "$TMP/instance2.json"
{ "b": 2 }
EOF

echo '{ "c": 3 }' | "$1" validate "$TMP/schema.json" \
  "$TMP/instance1.json" - "$TMP/instance2.json" --verbose 2>"$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/instance1.json
  matches $(realpath "$TMP")/schema.json
ok: <stdin>
  matches $(realpath "$TMP")/schema.json
ok: $(realpath "$TMP")/instance2.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
