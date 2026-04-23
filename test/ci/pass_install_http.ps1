$ErrorActionPreference = "Stop"
$CLI = $args[0]
$TMP = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $TMP | Out-Null

function Check-Diff($actual, $expected) {
    $actualNorm = ($actual -replace "`r`n", "`n").TrimEnd()
    $expectedNorm = ($expected -replace "`r`n", "`n").TrimEnd()
    if ($actualNorm -cne $expectedNorm) {
        Write-Host "EXPECTED:"
        Write-Host $expected
        Write-Host "---"
        Write-Host "ACTUAL:"
        Write-Host $actual
        throw "Output mismatch"
    }
}

function Run-CLI($arguments, $workingDirectory) {
    $pinfo = [System.Diagnostics.ProcessStartInfo]@{
        FileName = $CLI
        Arguments = $arguments
        RedirectStandardOutput = $true
        RedirectStandardError = $true
        UseShellExecute = $false
    }
    if ($workingDirectory) {
        $pinfo.WorkingDirectory = $workingDirectory
    }
    $process = [System.Diagnostics.Process]::Start($pinfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    return @{
        Stdout = $stdout
        Stderr = $stderr
        ExitCode = $process.ExitCode
    }
}

try {
    $projectDir = Join-Path $TMP "project"
    New-Item -ItemType Directory -Path $projectDir | Out-Null

    [System.IO.File]::WriteAllText((Join-Path $projectDir "jsonschema.json"), @'
{
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response": "./vendor/response.json"
  }
}
'@)

    $result = Run-CLI "install" $projectDir

    if ($result.ExitCode -ne 0) {
        throw "Expected exit code 0, got $($result.ExitCode). Stderr: $($result.Stderr)"
    }

    $resolvedTmp = (Resolve-Path $TMP).Path.TrimEnd('\')
    $output = $result.Stderr
    $expectedOutput = @"
Fetching       : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response
Installed      : $resolvedTmp\project\vendor\response.json
"@

    [System.IO.File]::WriteAllText((Join-Path $TMP "output.txt"), $output)
    [System.IO.File]::WriteAllText((Join-Path $TMP "expected.txt"), $expectedOutput)
    Check-Diff $output $expectedOutput

    $responseFile = Join-Path (Join-Path $projectDir "vendor") "response.json"
    $actualSchema = [System.IO.File]::ReadAllText($responseFile)
    $expectedSchema = @'
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
'@

    Check-Diff $actualSchema $expectedSchema

    $fileBytes = [System.IO.File]::ReadAllBytes($responseFile)
    $hashBytes = [System.Security.Cryptography.SHA256]::Create().ComputeHash($fileBytes)
    $hash = -join ($hashBytes | ForEach-Object { $_.ToString("x2") })

    $lockFile = Join-Path $projectDir "jsonschema.lock.json"
    $actualLock = [System.IO.File]::ReadAllText($lockFile)
    $expectedLock = @"
{
  "version": 1,
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/response": {
      "path": "./vendor/response.json",
      "hash": "$hash",
      "hashAlgorithm": "sha256"
    }
  }
}
"@

    Check-Diff $actualLock $expectedLock

    $instance = Join-Path $TMP "instance.json"
    [System.IO.File]::WriteAllText($instance, @'
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": 42
}
'@)

    $validateResult = Run-CLI "validate `"$responseFile`" `"$instance`""
    if ($validateResult.ExitCode -ne 0) {
        throw "Validation failed with exit code $($validateResult.ExitCode). Stderr: $($validateResult.Stderr)"
    }
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
