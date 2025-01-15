#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: http://json-schema.org/draft-06/schema#
type: string
enum: [ foo ]
EOF

"$1" lint "$TMP/schema.yaml" --fix >"$TMP/stderr.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
The --fix option is not supported for YAML input files
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
