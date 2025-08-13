#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" lint --list --only definitions_to_defs > "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
definitions_to_defs
  `definitions` was superseded by `$defs` in 2019-09 and later versions

Number of rules: 1
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
