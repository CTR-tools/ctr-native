<#
.SYNOPSIS
    Packages the built Windows binary as the release archive.

.DESCRIPTION
    Collects the binary, the third-party notices and a README into
    a flat directory, zips it, and writes a sha256 beside the archive.

    config.ini and controls.ini are produced by copying default_config.ini and
    default_controls.ini. Every copy is written out source to destination on
    purpose.

.PARAMETER Version
    The release version, with or without the leading v.

.PARAMETER Executable
    The built binary, relative to the repository root.

.PARAMETER DistDir
    Where the archive and its checksum are written, relative to the repository
    root.

.EXAMPLE
    ./scripts/package-windows.ps1 -Version 1.2.0

    Packages build\ctr_native_expanded.exe as dist\ctr-native-expanded-v-1-2-0-windows-x86.zip.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Version,

    [string] $Executable = 'build\ctr_native_expanded.exe',

    [string] $DistDir = 'dist'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

# X.Y.Z only. This pipeline does not cut pre-release tags, so accepting one here
# would only produce an archive name nothing downstream agrees on.
$versionNumber = $Version -replace '^v', ''
if ($versionNumber -notmatch '^[0-9]+\.[0-9]+\.[0-9]+$') {
    throw "Version '$Version' is not X.Y.Z."
}

$executableName = 'ctr_native_expanded.exe'

# Matches the archives published so far, such as
# ctr-native-expanded-v-1-0-0-windows-x86.zip.
$packageName = 'ctr-native-expanded-v-{0}-windows-x86' -f ($versionNumber -replace '\.', '-')

$staging = Join-Path ([System.IO.Path]::GetTempPath()) ('ctr-package-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $staging -Force | Out-Null

try {
    $copies = @(
        @{ From = $Executable; To = $executableName }
        @{ From = 'THIRD_PARTY_NOTICES.md'; To = 'THIRD_PARTY_NOTICES.md' }
        @{ From = 'default_config.ini'; To = 'config.ini' }
        @{ From = 'default_controls.ini'; To = 'controls.ini' }
    )
    foreach ($copy in $copies) {
        $source = Join-Path $RepoRoot $copy.From
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Missing $source."
        }
        Copy-Item -LiteralPath $source -Destination (Join-Path $staging $copy.To) -Force
    }

    # The version is the one thing in the README that is not known until release
    # time, so the committed text carries a placeholder rather than being edited
    # per release.
    $readmeSource = Join-Path $RepoRoot 'packaging\README-windows.txt'
    if (-not (Test-Path -LiteralPath $readmeSource)) {
        throw "Missing $readmeSource."
    }
    $readme = (Get-Content -LiteralPath $readmeSource -Raw).Replace('{{VERSION}}', $versionNumber)
    # WriteAllText keeps the file's own trailing newline and encodes without a
    # BOM, which Set-Content would not do identically across PowerShell editions.
    [System.IO.File]::WriteAllText((Join-Path $staging 'README.txt'), $readme)

    $dist = Join-Path $RepoRoot $DistDir
    New-Item -ItemType Directory -Path $dist -Force | Out-Null
    $archivePath = Join-Path $dist "$packageName.zip"
    if (Test-Path -LiteralPath $archivePath) {
        Remove-Item -LiteralPath $archivePath -Force
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $staging,
        $archivePath,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $false)

    $hash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
    Set-Content -LiteralPath "$archivePath.sha256" -Value "$hash  $packageName.zip" -Encoding ascii

    Write-Host "Wrote $archivePath"
    Write-Host "Wrote $archivePath.sha256"
    Get-ChildItem -LiteralPath $staging | ForEach-Object { Write-Host "  $($_.Name)" }
}
finally {
    Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
}