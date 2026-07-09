#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
false
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate --default-dialect "https://json-schema.org/draft/2020-12/schema" \
  "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
