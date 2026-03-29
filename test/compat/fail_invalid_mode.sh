#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/base.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/candidate.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" compat "$TMP/base.json" "$TMP/candidate.json" --mode sideways \
  2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: Unknown compatibility mode
  at option mode
  with values
  - backward
  - forward
  - full

Run the `help` command for usage information
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" compat "$TMP/base.json" "$TMP/candidate.json" --mode sideways --json \
  > "$TMP/stdout.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
{
  "error": "Unknown compatibility mode",
  "option": "mode",
  "values": [ "backward", "forward", "full" ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
