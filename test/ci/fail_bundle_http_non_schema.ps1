$ErrorActionPreference = "Stop"
$CLI = $args[0]
$TMP = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $TMP | Out-Null
try {
    $schema = Join-Path $TMP "schema.json"

    [System.IO.File]::WriteAllText($schema, @'
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "allOf": [ { "$ref": "https://schemas.sourcemeta.com/self/v1/api/schemas/stats/jsonschema/2020-12/schema" } ]
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

    if ($process.ExitCode -ne 4) {
        throw "Expected exit code 4, got $($process.ExitCode)"
    }

    $resolvedTmp = (Resolve-Path $TMP).Path.TrimEnd('\')
    $expected = @"
error: The JSON document is not a valid JSON Schema
  at identifier https://schemas.sourcemeta.com/self/v1/api/schemas/stats/jsonschema/2020-12/schema
  at file path $resolvedTmp\schema.json
  at location "/allOf/0/`$ref"
"@

    [System.IO.File]::WriteAllText((Join-Path $TMP "stderr.txt"), $stderr)
    [System.IO.File]::WriteAllText((Join-Path $TMP "expected.txt"), $expected)

    $actualNorm = ($stderr -replace "`r`n", "`n").TrimEnd()
    $expectedNorm = ($expected -replace "`r`n", "`n").TrimEnd()
    if ($actualNorm -cne $expectedNorm) {
        Write-Host "EXPECTED:"
        Write-Host $expected
        Write-Host "---"
        Write-Host "ACTUAL:"
        Write-Host $stderr
        throw "Stderr output mismatch"
    }
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
