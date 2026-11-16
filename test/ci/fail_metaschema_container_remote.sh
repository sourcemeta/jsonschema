#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5894
mkdir "$TMP/server"
npx --yes http-server --version
npx --yes http-server "$TMP/server" --port "$PORT" &
SERVER_PID="$!"

clean() {
  kill "$SERVER_PID" 2>/dev/null || true
  rm -rf "$TMP"
}

trap clean EXIT

cat << EOF > "$TMP/server/metaschema.json"
{
  "\$id": "http://localhost:${PORT}/metaschema.json",
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$vocabulary": { "https://json-schema.org/draft/2020-12/vocab/core": true },
  "\$dynamicAnchor": "meta",
  "\$defs": [ 1, 2 ],
  "\$ref": "https://json-schema.org/draft/2020-12/meta/core"
}
EOF

cat << EOF > "$TMP/schema.json"
{
  "\$schema": "http://localhost:${PORT}/metaschema.json",
  "\$defs": {
    "unrelated": { "type": "number" }
  },
  "type": "string"
}
EOF

sleep 3

"$1" metaschema "$TMP/schema.json" --http \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Could not bundle to a container that is not an object
  at location "/\$defs"
  at url http://localhost:${PORT}/metaschema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" metaschema "$TMP/schema.json" --http --json \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Could not bundle to a container that is not an object",
  "location": "/\$defs",
  "url": "http://localhost:${PORT}/metaschema.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
