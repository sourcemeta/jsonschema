#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response": "./vendor/response.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response
Installed      : $(realpath "$TMP")/project/vendor/response.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response",
  "title": "JSON-RPC 2.0 Response Object",
  "description": "An object that represents the result of a method invocation on the server",
  "examples": [
    {
      "id": 1,
      "jsonrpc": "2.0",
      "result": 19
    },
    {
      "id": "req-002",
      "jsonrpc": "2.0",
      "result": [ 7, 8, 9 ]
    },
    {
      "id": 3,
      "error": {
        "code": -32601,
        "message": "Method not found"
      },
      "jsonrpc": "2.0"
    }
  ],
  "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
  "x-links": [ "https://www.jsonrpc.org/specification#response_object" ],
  "type": "object",
  "anyOf": [
    {
      "required": [ "jsonrpc", "id", "error" ],
      "properties": {
        "id": {
          "description": "The response identifier",
          "$ref": "./identifier"
        },
        "error": {
          "description": "The error object",
          "$ref": "./error"
        },
        "jsonrpc": {
          "description": "The protocol version",
          "const": "2.0"
        },
        "result": {
          "description": "Must not be included",
          "not": true
        }
      }
    },
    {
      "required": [ "jsonrpc", "id", "result" ],
      "properties": {
        "id": {
          "description": "The response identifier",
          "$ref": "./identifier"
        },
        "error": {
          "description": "Must not be included",
          "not": true
        },
        "jsonrpc": {
          "description": "The protocol version",
          "const": "2.0"
        },
        "result": {
          "description": "The method result"
        }
      }
    }
  ],
  "$defs": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/code-predefined": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/code-predefined",
      "title": "JSON-RPC 2.0 Predefined Error Code",
      "description": "An error code from the predefined range reserved by the JSON-RPC 2.0 specification",
      "examples": [ -32700, -32600, -32601, -32602, -32603, -32000 ],
      "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
      "x-links": [ "https://www.jsonrpc.org/specification#error_object" ],
      "type": "integer",
      "anyOf": [
        {
          "title": "Parse error",
          "description": "Invalid JSON was received by the server",
          "const": -32700
        },
        {
          "title": "Invalid request",
          "description": "The JSON sent is not a valid Request object",
          "const": -32600
        },
        {
          "title": "Method not found",
          "description": "The method does not exist / is not available",
          "const": -32601
        },
        {
          "title": "Invalid params",
          "description": "Invalid method parameter(s)",
          "const": -32602
        },
        {
          "title": "Internal error",
          "description": "Internal JSON-RPC error",
          "const": -32603
        },
        {
          "title": "Server error",
          "description": "Implementation-defined server error",
          "maximum": -32000,
          "minimum": -32099
        }
      ]
    },
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/code": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/code",
      "title": "JSON-RPC 2.0 Error Code",
      "description": "A number that indicates the error type that occurred",
      "examples": [ -32700, -32600, -32601, -32000, 100, -1 ],
      "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
      "x-links": [ "https://www.jsonrpc.org/specification#error_object" ],
      "anyOf": [
        {
          "$ref": "./code-predefined"
        },
        {
          "type": "integer"
        }
      ]
    },
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/error": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/error",
      "title": "JSON-RPC 2.0 Error Object",
      "description": "An object that describes an error that occurred during the request",
      "examples": [
        {
          "code": -32600,
          "message": "Invalid Request"
        },
        {
          "code": -32601,
          "message": "Method not found"
        },
        {
          "code": -32000,
          "data": {
            "details": "Database connection failed"
          },
          "message": "Server error"
        }
      ],
      "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
      "x-links": [ "https://www.jsonrpc.org/specification#error_object" ],
      "type": "object",
      "required": [ "code", "message" ],
      "properties": {
        "code": {
          "description": "The error type",
          "$ref": "./code"
        },
        "data": {
          "description": "Additional error information"
        },
        "message": {
          "description": "The error description",
          "type": "string"
        }
      }
    },
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier",
      "title": "JSON-RPC 2.0 Identifier",
      "description": "An identifier established by the client that must be matched in the response if included in a request",
      "examples": [ "abc123", 42, null, "request-1" ],
      "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
      "x-links": [ "https://www.jsonrpc.org/specification#request_object" ],
      "type": [ "string", "integer", "null" ]
    }
  }
}
EOF

diff "$TMP/project/vendor/response.json" "$TMP/expected_schema.json"

HASH="$(shasum -a 256 < "$TMP/project/vendor/response.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response": {
      "path": "./vendor/response.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"

cat << 'EOF' > "$TMP/instance.json"
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": 42
}
EOF

"$1" validate "$TMP/project/vendor/response.json" "$TMP/instance.json"
