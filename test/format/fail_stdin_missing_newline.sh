#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# Write schema without trailing newline by stripping it from heredoc output
cat << 'EOF' > "$TMP/schema_noeol.json"
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"}
EOF
printf '%s' "$(cat "$TMP/schema_noeol.json")" \
  | "$1" fmt --check - > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
fail: /dev/stdin

Run the `fmt` command without `--check/-c` to fix the formatting
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
printf '%s' "$(cat "$TMP/schema_noeol.json")" \
  | "$1" fmt --check - --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

cat << 'EOF' > "$TMP/expected.txt"
{
  "valid": false,
  "errors": [ "/dev/stdin" ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
