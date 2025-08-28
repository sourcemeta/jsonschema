#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [
    { "$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json" }
  ]
}
EOF

"$1" bundle "$TMP/schema.json" --http > "$TMP/result.json"

cat << EOF > "$TMP/expected.json"
{
  "\$schema": "http://json-schema.org/draft-07/schema#",
  "\$id": "file://$(realpath "$TMP")/schema.json",
  "allOf": [
    {
      "\$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json"
    }
  ],
  "definitions": {
    "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json": {
      "\$schema": "http://json-schema.org/draft-04/schema#",
      "id": "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json",
      "description": "Core schema meta-schema",
      "default": {},
      "type": "object",
      "properties": {
        "\$schema": {
          "type": "string"
        },
        "id": {
          "type": "string"
        },
        "title": {
          "type": "string"
        },
        "description": {
          "type": "string"
        },
        "default": {},
        "type": {
          "anyOf": [
            {
              "\$ref": "#/definitions/simpleTypes"
            },
            {
              "type": "array",
              "minItems": 1,
              "uniqueItems": true,
              "items": {
                "\$ref": "#/definitions/simpleTypes"
              }
            }
          ]
        },
        "enum": {
          "type": "array",
          "minItems": 1,
          "uniqueItems": true
        },
        "allOf": {
          "\$ref": "#/definitions/schemaArray"
        },
        "anyOf": {
          "\$ref": "#/definitions/schemaArray"
        },
        "oneOf": {
          "\$ref": "#/definitions/schemaArray"
        },
        "not": {
          "\$ref": ""
        },
        "exclusiveMaximum": {
          "default": false,
          "type": "boolean"
        },
        "maximum": {
          "type": "number"
        },
        "exclusiveMinimum": {
          "default": false,
          "type": "boolean"
        },
        "minimum": {
          "type": "number"
        },
        "multipleOf": {
          "type": "number",
          "exclusiveMinimum": true,
          "minimum": 0
        },
        "pattern": {
          "type": "string",
          "format": "regex"
        },
        "format": {
          "type": "string"
        },
        "maxLength": {
          "\$ref": "#/definitions/positiveInteger"
        },
        "minLength": {
          "\$ref": "#/definitions/positiveIntegerDefault0"
        },
        "maxItems": {
          "\$ref": "#/definitions/positiveInteger"
        },
        "minItems": {
          "\$ref": "#/definitions/positiveIntegerDefault0"
        },
        "uniqueItems": {
          "default": false,
          "type": "boolean"
        },
        "items": {
          "default": {},
          "anyOf": [
            {
              "\$ref": ""
            },
            {
              "\$ref": "#/definitions/schemaArray"
            }
          ]
        },
        "additionalItems": {
          "default": {},
          "anyOf": [
            {
              "type": "boolean"
            },
            {
              "\$ref": ""
            }
          ]
        },
        "required": {
          "\$ref": "#/definitions/stringArray"
        },
        "maxProperties": {
          "\$ref": "#/definitions/positiveInteger"
        },
        "minProperties": {
          "\$ref": "#/definitions/positiveIntegerDefault0"
        },
        "properties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": ""
          }
        },
        "patternProperties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": ""
          }
        },
        "additionalProperties": {
          "default": {},
          "anyOf": [
            {
              "type": "boolean"
            },
            {
              "\$ref": ""
            }
          ]
        },
        "dependencies": {
          "type": "object",
          "additionalProperties": {
            "anyOf": [
              {
                "\$ref": ""
              },
              {
                "\$ref": "#/definitions/stringArray"
              }
            ]
          }
        },
        "definitions": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": ""
          }
        }
      },
      "dependencies": {
        "exclusiveMaximum": [ "maximum" ],
        "exclusiveMinimum": [ "minimum" ]
      },
      "definitions": {
        "positiveInteger": {
          "type": "integer",
          "minimum": 0
        },
        "positiveIntegerDefault0": {
          "allOf": [
            {
              "\$ref": "#/definitions/positiveInteger"
            },
            {
              "default": 0
            }
          ]
        },
        "schemaArray": {
          "type": "array",
          "minItems": 1,
          "items": {
            "\$ref": ""
          }
        },
        "simpleTypes": {
          "enum": [
            "array",
            "boolean",
            "integer",
            "null",
            "number",
            "object",
            "string"
          ]
        },
        "stringArray": {
          "type": "array",
          "minItems": 1,
          "uniqueItems": true,
          "items": {
            "type": "string"
          }
        }
      }
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
