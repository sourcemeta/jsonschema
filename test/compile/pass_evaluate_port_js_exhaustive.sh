#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": [ "name" ]
}
EOF

"$1" compile "$TMP/schema.json" > "$TMP/template.json"

BLAZE_JS="$(dirname "$0")/../../vendor/blaze/ports/javascript/index.mjs"

cat << EOF > "$TMP/test.mjs"
import { strict as assert } from "node:assert";
import { readFileSync } from "node:fs";
import { Blaze } from "$BLAZE_JS";

const template = JSON.parse(readFileSync("$TMP/template.json", "utf-8"));
const evaluator = new Blaze(template);

assert(evaluator.validate({ "name": "John", "age": 30 }));
assert(!evaluator.validate({ "age": "not a number" }));
EOF

node "$TMP/test.mjs"
