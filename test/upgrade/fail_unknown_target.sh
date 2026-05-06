#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" upgrade --to fooo "$TMP/schema.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected.txt"
error: The given target dialect is not supported
  at option to
  with values
  - draft4
  - draft6
  - draft7
  - 2019-09
  - 2020-12

Run the `help` command for usage information
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" upgrade --to fooo "$TMP/schema.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Invalid CLI arguments
test "$EXIT_CODE" = "5"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "The given target dialect is not supported",
  "option": "to",
  "values": [ "draft4", "draft6", "draft7", "2019-09", "2020-12" ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
