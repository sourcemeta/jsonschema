#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
mkdir "$TMP/server"
npx --yes http-server --version
npx --yes http-server "$TMP/server" --port 8000 &
SERVER_PID="$!"

clean() {
  kill "$SERVER_PID"
  rm -rf "$TMP";
}

trap clean EXIT

cat << 'EOF' > "$TMP/server/schema.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
type: string
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "http://localhost:8000/schema.yaml"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

sleep 3

"$1" validate "$TMP/schema.json" --http "$TMP/instance.json" --verbose
