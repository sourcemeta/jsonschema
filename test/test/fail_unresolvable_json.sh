#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com/unknown",
  "tests": [
    {
      "valid": true,
      "data": {}
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --json > "$TMP/output.json" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "error": "Could not resolve the reference to an external schema",
  "identifier": "https://example.com/unknown",
  "filePath": "$(realpath "$TMP")/test.json"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
