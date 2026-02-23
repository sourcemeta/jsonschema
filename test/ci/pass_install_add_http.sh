#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cd "$TMP/project"
"$1" install "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier" \
  "./vendor/identifier.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Adding         : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier -> ./vendor/identifier.json
Fetching       : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier
Installed      : $(realpath "$TMP")/project/vendor/identifier.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_config.json"
{
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier": "./vendor/identifier.json"
  }
}
EOF

diff "$TMP/project/jsonschema.json" "$TMP/expected_config.json"

cat << 'EOF' > "$TMP/expected_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier",
  "title": "JSON-RPC 2.0 Identifier",
  "description": "An identifier established by the client that must be matched in the response if included in a request",
  "examples": [ "abc123", 42, null, "request-1" ],
  "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
  "x-links": [ "https://www.jsonrpc.org/specification#request_object" ],
  "type": [ "string", "integer", "null" ]
}
EOF

diff "$TMP/project/vendor/identifier.json" "$TMP/expected_schema.json"

HASH="$(shasum -a 256 < "$TMP/project/vendor/identifier.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier": {
      "path": "./vendor/identifier.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/project/vendor/identifier.json" "$TMP/instance.json"
