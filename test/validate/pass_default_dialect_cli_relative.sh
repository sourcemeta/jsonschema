#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/sub"

cat << 'EOF' > "$TMP/meta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/my-meta",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true
  }
}
EOF

cat << 'EOF' > "$TMP/sub/schema.json"
{
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/sub/instance.json"
"hello"
EOF

cd "$TMP/sub"

"$1" validate schema.json instance.json \
  --default-dialect ../meta.json --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$TMP")/sub/instance.json
  matches $(realpath "$TMP")/sub/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
