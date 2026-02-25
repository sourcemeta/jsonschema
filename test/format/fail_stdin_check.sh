#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" fmt --check - >"$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
fail: (stdin)

Run the `fmt` command without `--check/-c` to fix the formatting
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
