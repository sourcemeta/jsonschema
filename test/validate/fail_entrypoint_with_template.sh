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

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

cat << 'EOF' > "$TMP/template.json"
[]
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint "/\$defs/Foo" --template "$TMP/template.json" \
  > "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The --entrypoint option cannot be used with --template
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint "/\$defs/Foo" --template "$TMP/template.json" --json \
  > "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The --entrypoint option cannot be used with --template"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
