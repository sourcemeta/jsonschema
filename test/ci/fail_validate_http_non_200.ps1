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
  "allOf": [ { "$ref": "https://one.sourcemeta.com" } ]
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

    if ($process.ExitCode -ne 6) {
        throw "Expected exit code 6, got $($process.ExitCode)"
    }

    $expected = "error: Failed to parse the JSON document`n  at line 2`n  at column 1`n"

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
