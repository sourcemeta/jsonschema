$ErrorActionPreference = "Stop"
$CLI = $args[0]
$TMP = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $TMP | Out-Null

function Check-Diff($actual, $expected) {
    $actualNorm = ($actual -replace "`r`n", "`n").TrimEnd()
    $expectedNorm = ($expected -replace "`r`n", "`n").TrimEnd()
    if ($actualNorm -ne $expectedNorm) {
        Write-Host "EXPECTED:"
        Write-Host $expected
        Write-Host "---"
        Write-Host "ACTUAL:"
        Write-Host $actual
        throw "Output mismatch"
    }
}

try {
    $schema = Join-Path $TMP "schema.json"

    [System.IO.File]::WriteAllText($schema, @'
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [
    { "$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json" }
  ]
}
'@)

    $pinfo = [System.Diagnostics.ProcessStartInfo]@{
        FileName = $CLI
        Arguments = "bundle `"$schema`" --http"
        RedirectStandardOutput = $true
        RedirectStandardError = $true
        UseShellExecute = $false
    }
    $process = [System.Diagnostics.Process]::Start($pinfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($process.ExitCode -ne 0) {
        throw "Expected exit code 0, got $($process.ExitCode). Stderr: $stderr"
    }

    $resolvedTmpUri = (Resolve-Path $TMP).Path.TrimEnd('\') -replace '\\', '/'

    [System.IO.File]::WriteAllText((Join-Path $TMP "result.json"), $stdout)

    $expected = (@'
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "file:///__RESOLVED__/schema.json",
  "title": "Test",
  "description": "Test schema",
  "allOf": [
    {
      "$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema"
    }
  ],
  "definitions": {
    "https://schemas.sourcemeta.com/jsonschema/draft4/schema": {
      "$schema": "http://json-schema.org/draft-04/schema#",
      "id": "https://schemas.sourcemeta.com/jsonschema/draft4/schema",
      "description": "Core schema meta-schema",
      "default": {},
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "$schema": {
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
          "$ref": "#/definitions/positiveInteger"
        },
        "minLength": {
          "$ref": "#/definitions/positiveIntegerDefault0"
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
              "$ref": "#"
            }
          ]
        },
        "items": {
          "default": {},
          "anyOf": [
            {
              "$ref": "#"
            },
            {
              "$ref": "#/definitions/schemaArray"
            }
          ]
        },
        "maxItems": {
          "$ref": "#/definitions/positiveInteger"
        },
        "minItems": {
          "$ref": "#/definitions/positiveIntegerDefault0"
        },
        "uniqueItems": {
          "default": false,
          "type": "boolean"
        },
        "maxProperties": {
          "$ref": "#/definitions/positiveInteger"
        },
        "minProperties": {
          "$ref": "#/definitions/positiveIntegerDefault0"
        },
        "required": {
          "$ref": "#/definitions/stringArray"
        },
        "additionalProperties": {
          "default": {},
          "anyOf": [
            {
              "type": "boolean"
            },
            {
              "$ref": "#"
            }
          ]
        },
        "definitions": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "$ref": "#"
          }
        },
        "properties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "$ref": "#"
          }
        },
        "patternProperties": {
          "default": {},
          "type": "object",
          "additionalProperties": {
            "$ref": "#"
          }
        },
        "dependencies": {
          "type": "object",
          "additionalProperties": {
            "anyOf": [
              {
                "$ref": "#"
              },
              {
                "$ref": "#/definitions/stringArray"
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
              "$ref": "#/definitions/simpleTypes"
            },
            {
              "type": "array",
              "minItems": 1,
              "uniqueItems": true,
              "items": {
                "$ref": "#/definitions/simpleTypes"
              }
            }
          ]
        },
        "format": {
          "type": "string"
        },
        "allOf": {
          "$ref": "#/definitions/schemaArray"
        },
        "anyOf": {
          "$ref": "#/definitions/schemaArray"
        },
        "oneOf": {
          "$ref": "#/definitions/schemaArray"
        },
        "not": {
          "$ref": "#"
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
            "$ref": "#"
          }
        },
        "positiveInteger": {
          "type": "integer",
          "minimum": 0
        },
        "positiveIntegerDefault0": {
          "allOf": [
            {
              "$ref": "#/definitions/positiveInteger"
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
'@) -replace '__RESOLVED__', $resolvedTmpUri

    [System.IO.File]::WriteAllText((Join-Path $TMP "expected.json"), $expected)

    Check-Diff $stdout $expected
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
