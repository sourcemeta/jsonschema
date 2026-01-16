#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/person",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "address": { "$ref": "https://example.com/address" }
  },
  "required": [ "name" ]
}
EOF

cat << 'EOF' > "$TMP/address.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/address",
  "type": "object",
  "properties": {
    "street": { "type": "string" },
    "city": { "type": "string" }
  },
  "required": [ "street", "city" ]
}
EOF

"$1" codegen "$TMP/schema.json" --resolve "$TMP/address.json" \
  --target typescript > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type SchemaName = string;

export type SchemaAddress = _SchemaAddress;

export type SchemaAddressStreet = string;

export type SchemaAddressCity = string;

export interface _SchemaAddress {
  "street": SchemaAddressStreet;
  "city": SchemaAddressCity;
  [key: string]: unknown | undefined;
}

export interface Schema {
  "name": SchemaName;
  "address"?: SchemaAddress;
  [key: string]: unknown | undefined;
}
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
