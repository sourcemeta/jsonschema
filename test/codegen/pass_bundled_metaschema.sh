#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com/schema",
  "type": "string",
  "$defs": {
    "https://example.com/meta": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/meta",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      },
      "type": "object"
    }
  }
}
EOF

"$1" codegen "$TMP/schema.json" --target typescript > "$TMP/result.txt" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type SchemaMeta = Record<string, unknown>;

export type Schema = string;
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
