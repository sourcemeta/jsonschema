#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
$id: https://example.com
$schema: https://json-schema.org/draft/2020-12/schema
type: string
EOF

"$1" fmt "$TMP/schema.custom" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: This command does not support YAML input files yet
  at file path $(realpath "$TMP")/schema.custom
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
