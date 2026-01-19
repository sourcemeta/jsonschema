#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

"$1" codegen "$TMP/schema.json" --target typescript --verbose \
  > "$TMP/result.txt" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type Schema = string;
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_log.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
EOF

diff "$TMP/output.txt" "$TMP/expected_log.txt"
