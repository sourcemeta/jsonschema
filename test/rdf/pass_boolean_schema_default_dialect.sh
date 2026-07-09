#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
true
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" \
  --default-dialect "https://json-schema.org/draft/2020-12/schema" \
  > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[]
EOF

diff "$TMP/output.json" "$TMP/expected.json"
