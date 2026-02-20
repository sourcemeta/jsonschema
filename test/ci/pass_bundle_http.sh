#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
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
  "title": "Test",
  "description": "Test schema",
  "allOf": [
    {
      "\$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema"
    }
  ],
  "definitions": {
    "https://schemas.sourcemeta.com/jsonschema/draft4/schema": {
      "\$schema": "http://json-schema.org/draft-04/schema#",
      "id": "https://schemas.sourcemeta.com/jsonschema/draft4/schema",
      "description": "Core schema meta-schema",
      "default": {},
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "\$schema": {
          "type": "string"
        },
        "title": {
          "type": "string"
        },
        "description": {
          "type": "string"
        },
        "default": {},
        "multipleOf": {
          "type": "number",
          "exclusiveMinimum": true,
          "minimum": 0
        },
        "maximum": {
          "type": "number"
        },
        "exclusiveMaximum": {
          "default": false,
          "type": "boolean"
        },
        "minimum": {
          "type": "number"
        },
        "exclusiveMinimum": {
          "default": false,
          "type": "boolean"
        },
        "maxLength": {
          "\$ref": "#/definitions/positiveInteger"
        },
        "minLength": {
          "\$ref": "#/definitions/positiveIntegerDefault0"
        },
        "pattern": {
          "type": "string",
          "format": "regex"
        },
        "additionalItems": {
          "default": {},
          "anyOf": [
            {
              "type": "boolean"
            },
            {
              "\$ref": "#"
            }
          ]
        },
        "items": {
          "default": {},
          "anyOf": [
            {
              "\$ref": "#"
            },
            {
              "\$ref": "#/definitions/schemaArray"
            }
          ]
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
        "maxProperties": {
          "\$ref": "#/definitions/positiveInteger"
        },
        "minProperties": {
          "\$ref": "#/definitions/positiveIntegerDefault0"
        },
        "required": {
          "\$ref": "#/definitions/stringArray"
        },
        "additionalProperties": {
          "default": {},
          "anyOf": [
            {
              "type": "boolean"
            },
            {
              "\$ref": "#"
            }
          ]
        },
        "definitions": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": "#"
          }
        },
        "properties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": "#"
          }
        },
        "patternProperties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "\$ref": "#"
          }
        },
        "dependencies": {
          "type": "object",
          "additionalProperties": {
            "anyOf": [
              {
                "\$ref": "#"
              },
              {
                "\$ref": "#/definitions/stringArray"
              }
            ]
          }
        },
        "enum": {
          "type": "array",
          "minItems": 1,
          "uniqueItems": true
        },
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
        "format": {
          "type": "string"
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
          "\$ref": "#"
        }
      },
      "dependencies": {
        "exclusiveMaximum": [ "maximum" ],
        "exclusiveMinimum": [ "minimum" ]
      },
      "definitions": {
        "schemaArray": {
          "type": "array",
          "minItems": 1,
          "items": {
            "\$ref": "#"
          }
        },
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
