#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": [ "name" ]
}
EOF

"$1" codegen "$TMP/schema.json" -t typescript > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type SchemaName = string;

export type SchemaAge = number;

export interface Schema {
  "name": SchemaName;
  "age"?: SchemaAge;
  [key: string]: unknown | undefined;
}
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
