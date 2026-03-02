#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# Write LF-only JSON to a temp file, then convert to CRLF via awk before piping
cat << 'EOF' > "$TMP/lf.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
awk '{ printf "%s\r\n", $0 }' "$TMP/lf.json" \
  | "$1" fmt - > "$TMP/output.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
