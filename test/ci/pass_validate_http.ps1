$ErrorActionPreference = "Stop"
$CLI = $args[0]
$TMP = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $TMP | Out-Null
try {
    $schema = Join-Path $TMP "schema.json"
    $instance = Join-Path $TMP "instance.json"

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

    [System.IO.File]::WriteAllText($instance, '{ "type": "string" }')

    $pinfo = [System.Diagnostics.ProcessStartInfo]@{
        FileName = $CLI
        Arguments = "validate `"$schema`" `"$instance`" --http"
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

    [System.IO.File]::WriteAllText((Join-Path $TMP "stderr.txt"), $stderr)
    [System.IO.File]::WriteAllText((Join-Path $TMP "expected.txt"), "")

    $actualNorm = ($stderr -replace "`r`n", "`n").TrimEnd()
    if ($actualNorm -cne "") {
        Write-Host "EXPECTED: (empty)"
        Write-Host "ACTUAL:"
        Write-Host $stderr
        throw "Expected empty stderr"
    }
} finally {
    Remove-Item -Recurse -Force $TMP -ErrorAction SilentlyContinue
}
