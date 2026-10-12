<#
.SYNOPSIS
    Checks that the built game reports the version CMake resolved for it.

.DESCRIPTION
    Runs the binary with --version and compares what it prints against what
    cmake/CtrNativeVersion.cmake resolves for the current checkout.

    The expectation is read out of the project's own CMake logic rather than
    reimplemented here, so this check cannot drift away from the rules the build
    actually follows. On top of that the 0.0.0-dev fallback is rejected outright,
    because matching a wrong version against a wrong expectation would still pass.

    Exits 0 when the two agree, and non-zero with an explanation otherwise.

.PARAMETER RepoRoot
    Repository root, which is also the working directory the version has to be
    resolved from. Defaults to the parent of this script's directory.

.PARAMETER Executable
    The built binary, relative to RepoRoot.

.EXAMPLE
    ./scripts/verify_version.ps1

    Checks build\ctr_native_expanded.exe against this checkout.
#>
[CmdletBinding()]
param(
    [string] $RepoRoot,
    [string] $Executable = 'build\ctr_native_expanded.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot) {
    $RepoRoot = Split-Path -Parent $PSScriptRoot
}

function Resolve-ExpectedVersion {
    <#
    .SYNOPSIS
        Asks cmake/CtrNativeVersion.cmake what version this checkout builds as.
    #>
    $tempRoot = if ($env:RUNNER_TEMP) {
        $env:RUNNER_TEMP
    }
    else {
        [System.IO.Path]::GetTempPath()
    }
    $probe = Join-Path $tempRoot 'ctr_expected_version.cmake'

    # A literal here-string, so PowerShell leaves the ${...} references alone for
    # CMake to expand when it reads the probe.
    @'
include("${CMAKE_SOURCE_DIR}/cmake/CtrNativeVersion.cmake")
message(STATUS "EXPECTED_VERSION=${CTR_NATIVE_EXPANDED_VERSION}")
message(STATUS "EXPECTED_BUILD_ID=${CTR_NATIVE_EXPANDED_BUILD_ID}")
'@ | Set-Content -LiteralPath $probe -Encoding ascii

    # `cmake -P` runs with the working directory as CMAKE_SOURCE_DIR, which is
    # where CtrNativeVersion.cmake looks for .git_archival.txt and runs git.
    Push-Location $RepoRoot
    try {
        $resolved = (& cmake -P $probe) | Out-String
        $exitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
        Remove-Item -LiteralPath $probe -ErrorAction SilentlyContinue
    }

    if ($exitCode -ne 0) {
        throw "cmake -P failed with exit code $exitCode."
    }

    # message(STATUS) goes to stdout, so this needs no stderr redirection.
    if ($resolved -notmatch '(?s)EXPECTED_VERSION=(?<Version>\S+).*EXPECTED_BUILD_ID=(?<BuildId>\S+)') {
        throw "Could not read the expected version out of CMake's output:`n$resolved"
    }

    return [pscustomobject]@{
        Version = $Matches.Version
        BuildId = $Matches.BuildId
    }
}

$exePath = Join-Path $RepoRoot $Executable
if (-not (Test-Path -LiteralPath $exePath)) {
    throw "No binary at $exePath. Build the game before checking its version."
}

$reported = (& $exePath --version) | Out-String
if ($LASTEXITCODE -ne 0) {
    throw "$Executable --version exited with $LASTEXITCODE."
}
$reported = $reported.Trim()

$expected = Resolve-ExpectedVersion

Write-Host "Binary reports: $reported"
Write-Host "CMake resolved: $($expected.Version) ($($expected.BuildId))"

if ($expected.Version -eq '0.0.0-dev') {
    throw "The version resolved to the 0.0.0-dev fallback, so no release tag was reachable. Check that the checkout fetched its tags."
}

$expectedLine = "CTR Native Expanded $($expected.Version) ($($expected.BuildId))"
if ($reported -ne $expectedLine) {
    throw "The binary does not report the version CMake resolved for it.`n  expected: $expectedLine`n  reported: $reported"
}

Write-Host "Version matches: $expectedLine"
