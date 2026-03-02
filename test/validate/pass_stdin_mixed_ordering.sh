#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance1.json"
{ "a": 1 }
EOF

cat << 'EOF' > "$TMP/instance2.json"
{ "b": 2 }
EOF

echo '{ "c": 3 }' | "$1" validate "$TMP/schema.json" \
  "$TMP/instance1.json" - "$TMP/instance2.json" --verbose 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "0" || exit 1

TMPREAL="$(cd "$TMP" && pwd -P)"

cat << EOF > "$TMP/expected.txt"
ok: $TMPREAL/instance1.json
  matches $TMPREAL/schema.json
ok: <stdin>
  matches $TMPREAL/schema.json
ok: $TMPREAL/instance2.json
  matches $TMPREAL/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
