#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
# shellcheck disable=SC2329,SC2317
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/server"

cat << 'EOF' > "$TMP/server/schema.json"
"not a schema"
EOF

PORT_FILE="$TMP/port.txt"
SERVER_LOG="$TMP/server.log"
SERVERPY="$(dirname "$(readlink -f "$0")")/http_server.py"
python3 "$SERVERPY" "$TMP/server" "$PORT_FILE" >"$SERVER_LOG" 2>&1 &
SERVER_PID="$!"

clean() {
  kill "$SERVER_PID" 2>/dev/null || true
  rm -rf "$TMP"
}
trap clean EXIT

TRIES=0
while [ ! -s "$PORT_FILE" ]; do
  if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat "$SERVER_LOG" >&2 || true
    exit 1
  fi
  TRIES=$((TRIES + 1))
  if [ "$TRIES" -gt 100 ]; then
    cat "$SERVER_LOG" >&2 || true
    exit 1
  fi
  sleep 0.1
done
PORT="$(cat "$PORT_FILE")"

"$1" metaschema "http://127.0.0.1:$PORT/schema.json" --http \
  2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The fetched document does not represent a valid JSON Schema
  at uri http://127.0.0.1:$PORT/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
