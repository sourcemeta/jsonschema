#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
PORT=5894

# Both halves of one exchange. The server records what actually reached the
# wire, which printing the request cannot show, and answers with an envelope
# shaped like the ones providers really send, chain of thought and all
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
      choices: [ {
        finish_reason: "stop",
        index: 0,
        message: {
          content: "{\n  \"capital\": \"Berlin\"\n}",
          reasoning_content: "The capital of Germany is Berlin.",
          refusal: null,
          role: "assistant"
        }
      } ],
      created: 1791216234,
      id: "resp_eb4a62d3-39eb-42fd-9231-c6650b2a0668",
      object: "chat.completion",
      usage: {
        completion_tokens: 128,
        completion_tokens_details: { reasoning_tokens: 116 },
        prompt_tokens: 17,
        total_tokens: 145
      }
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
  "type": "object",
  "properties": { "capital": { "type": "string" } },
  "required": [ "capital" ],
  "additionalProperties": false
}
EOF

# Read from standard input so that the identifier bundling gives the schema is
# the one standard input always goes by, rather than wherever this ran
"$1" llm - --ask "What is the capital of Germany?" \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  --header "Authorization: Bearer secret" \
  --param /max_tokens=64 \
  < "$TMP/schema.json" > "$TMP/output.txt" 2> "$TMP/error.txt"

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
        "content": "What is the capital of Germany?"
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
          "type": "object",
          "required": [
            "capital"
          ],
          "properties": {
            "capital": {
              "type": "string"
            }
          },
          "additionalProperties": false
        }
      }
    },
    "max_tokens": 64
  }
}
EOF

diff "$TMP/received.json" "$TMP/expected_received.json"

cat << 'EOF' > "$TMP/expected.txt"
{
  "capital": "Berlin"
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_error.txt"
tokens: 17 prompt, 128 completion, 145 total
EOF

diff "$TMP/error.txt" "$TMP/expected_error.txt"

"$1" llm - --ask "What is the capital of Germany?" \
  --url "http://localhost:${PORT}/v1/chat/completions" --model my-model \
  --json < "$TMP/schema.json" > "$TMP/output_json.txt" 2>&1

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "valid": true,
  "raw": "{\n  \"capital\": \"Berlin\"\n}",
  "document": {
    "capital": "Berlin"
  },
  "output": {
    "valid": true,
    "annotations": [
      {
        "keywordLocation": "/properties",
        "absoluteKeywordLocation": "tag:sourcemeta.com,2026:jsonschema/stdin#/properties",
        "instanceLocation": "",
        "instancePosition": [ 1, 1, 3, 1 ],
        "annotation": [ "capital" ]
      }
    ]
  },
  "finishReason": "stop",
  "usage": {
    "promptTokens": 17,
    "completionTokens": 128,
    "totalTokens": 145
  }
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
