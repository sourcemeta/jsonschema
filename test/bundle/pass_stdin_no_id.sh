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

mkdir -p "$TMP/nested/deeper"

# The identifier of a schema read from standard input must not depend on
# the directory the command happens to run from
cd "$TMP"
"$1" bundle - < "$TMP/schema.json" > "$TMP/first.json"

cd "$TMP/nested/deeper"
"$1" bundle - < "$TMP/schema.json" > "$TMP/second.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "tag:sourcemeta.com,2026:jsonschema/stdin",
  "type": "string"
}
EOF

diff "$TMP/first.json" "$TMP/expected.json"
diff "$TMP/second.json" "$TMP/expected.json"
