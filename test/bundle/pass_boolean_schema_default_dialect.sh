#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
true
EOF

"$1" bundle "$TMP/schema.json" \
  --default-dialect "https://json-schema.org/draft/2020-12/schema" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
true
EOF

diff "$TMP/result.json" "$TMP/expected.json"

# Must come out formatted
"$1" fmt "$TMP/result.json" --check --default-dialect "https://json-schema.org/draft/2020-12/schema"
