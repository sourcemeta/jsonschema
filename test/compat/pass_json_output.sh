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

"$1" compat "$TMP/base.json" "$TMP/candidate.json" --json \
  > "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

test ! -s "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
{
  "status": "prototype",
  "message": "This prototype validates inputs and reserves the CLI contract while the semantic compatibility engine is integrated.",
  "base": "$TMP/base.json",
  "candidate": "$TMP/candidate.json",
  "mode": "backward",
  "failOn": "breaking"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
