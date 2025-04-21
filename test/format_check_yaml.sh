#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
type: 1
$schema: http://json-schema.org/draft-04/schema#
EOF

"$1" fmt "$TMP/schema.yaml" --check 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/error.txt"
error: This command does not support YAML input files yet
EOF

diff "$TMP/stderr.txt" "$TMP/error.txt"
