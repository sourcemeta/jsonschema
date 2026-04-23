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

    $result = Run-CLI "install `"https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier`" `"./vendor/identifier.json`"" $projectDir

    if ($result.ExitCode -ne 0) {
        throw "Expected exit code 0, got $($result.ExitCode). Stderr: $($result.Stderr)"
    }

    $resolvedTmp = (Resolve-Path $TMP).Path.TrimEnd('\')
    $output = $result.Stderr
    $expectedOutput = @"
Adding         : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier -> ./vendor/identifier.json
Fetching       : https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier
Installed      : $resolvedTmp\project\vendor\identifier.json
"@

    [System.IO.File]::WriteAllText((Join-Path $TMP "output.txt"), $output)
    [System.IO.File]::WriteAllText((Join-Path $TMP "expected.txt"), $expectedOutput)
    Check-Diff $output $expectedOutput

    $configFile = Join-Path $projectDir "jsonschema.json"
    $actualConfig = [System.IO.File]::ReadAllText($configFile)
    $expectedConfig = @'
{
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier": "./vendor/identifier.json"
  }
}
'@

    Check-Diff $actualConfig $expectedConfig

    $identifierFile = Join-Path (Join-Path $projectDir "vendor") "identifier.json"
    $actualSchema = [System.IO.File]::ReadAllText($identifierFile)
    $expectedSchema = @'
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier",
  "title": "JSON-RPC 2.0 Identifier",
  "description": "An identifier established by the client that must be matched in the response if included in a request",
  "examples": [ "abc123", 42, null, "request-1" ],
  "x-license": "https://github.com/sourcemeta/std/blob/main/LICENSE",
  "x-links": [ "https://www.jsonrpc.org/specification#request_object" ],
  "type": [ "string", "integer", "null" ]
}
'@

    Check-Diff $actualSchema $expectedSchema

    $hash = (Get-FileHash -Path $identifierFile -Algorithm SHA256).Hash.ToLower()

    $lockFile = Join-Path $projectDir "jsonschema.lock.json"
    $actualLock = [System.IO.File]::ReadAllText($lockFile)
    $expectedLock = @"
{
  "version": 1,
  "dependencies": {
    "https://schemas.sourcemeta.com/sourcemeta/std/v0/jsonrpc/v2.0/identifier": {
      "path": "./vendor/identifier.json",
      "hash": "$hash",
      "hashAlgorithm": "sha256"
    }
  }
}
"@

    Check-Diff $actualLock $expectedLock

    $instance = Join-Path $TMP "instance.json"
    [System.IO.File]::WriteAllText($instance, '"hello"')

    $validateResult = Run-CLI "validate `"$identifierFile`" `"$instance`""
    if ($validateResult.ExitCode -ne 0) {
        throw "Validation failed with exit code $($validateResult.ExitCode). Stderr: $($validateResult.Stderr)"
    }
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
