#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5895

# What actually reaches the wire, which printing the request cannot show: the
# method, the content type, the credential a header carries, and the body the
# schema turned into
cat << 'EOF' > "$TMP/server.js"
const filesystem = require('fs');
const http = require('http');
const server = http.createServer((request, response) => {
  let body = '';
  request.on('data', (chunk) => { body += chunk; });
  request.on('end', () => {
    filesystem.writeFileSync(process.argv[3], JSON.stringify({
      method: request.method,
      contentType: request.headers['content-type'],
      authorization: request.headers['authorization'],
      body: JSON.parse(body)
    }, null, 2) + '\n');
    response.setHeader('content-type', 'application/json');
    response.end(JSON.stringify({
      choices: [ { finish_reason: "stop", message: { content: "\"Berlin\"", role: "assistant" } } ]
    }));
  });
});
server.listen(parseInt(process.argv[2], 10));
EOF

node "$TMP/server.js" "$PORT" "$TMP/received.json" &
SERVER_PID="$!"

clean() {
  kill "$SERVER_PID" 2>/dev/null || true
  rm -rf "$TMP"
}
trap clean EXIT

sleep 2

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

# Read from standard input so that the identifier bundling gives the schema is
# the one standard input always goes by, rather than wherever this ran
"$1" llm - --ask "Name a capital" \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  --header "Authorization: Bearer secret" \
  --param /max_tokens=64 \
  < "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
"Berlin"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_received.json"
{
  "method": "POST",
  "contentType": "application/json",
  "authorization": "Bearer secret",
  "body": {
    "model": "my-model",
    "messages": [
      {
        "role": "user",
        "content": "Name a capital"
      }
    ],
    "response_format": {
      "type": "json_schema",
      "json_schema": {
        "name": "schema",
        "strict": true,
        "schema": {
          "$schema": "https://json-schema.org/draft/2020-12/schema",
          "$id": "tag:sourcemeta.com,2026:jsonschema/stdin",
          "type": "string"
        }
      }
    },
    "max_tokens": 64
  }
}
EOF

diff "$TMP/received.json" "$TMP/expected_received.json"
