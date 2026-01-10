#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
# shellcheck disable=SC2329,SC2317
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

"$1" validate "https://example.com/schema.json" "$TMP/instance.json" \
  2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: Remote schema inputs require network access. Pass `--http/-h`
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" validate "https://example.com/schema.json" "$TMP/instance.json" --json \
  >"$TMP/stdout.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.json"
{
  "error": "Remote schema inputs require network access. Pass `--http/-h`"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.json"
