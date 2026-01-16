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
    "address": { "$ref": "./schemas/address.json" }
  },
  "required": [ "name" ]
}
EOF

mkdir -p "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/address.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "street": { "type": "string" },
    "city": { "type": "string" }
  },
  "required": [ "street", "city" ]
}
EOF

"$1" codegen "$TMP/schema.json" --name Person --target typescript \
  > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type PersonName = string;

export type PersonAddress = _PersonAddress;

export type PersonAddressStreet = string;

export type PersonAddressCity = string;

export interface _PersonAddress {
  "street": PersonAddressStreet;
  "city": PersonAddressCity;
  [key: string]: unknown | undefined;
}

export interface Person {
  "name": PersonName;
  "address"?: PersonAddress;
  [key: string]: unknown | undefined;
}
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
