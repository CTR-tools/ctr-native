<# Installs/verifies local track tools. No game data or patches are downloaded. #>
[CmdletBinding()]
param(
    [string] $Assets = '',
    [string] $Cache = '',
    [switch] $VerifyOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if ($Assets -eq '') { $Assets = Join-Path $repo 'assets' }
if ($Cache -eq '') { $Cache = Join-Path $repo '.cache/track-tools' }
$tools = Join-Path $Assets 'tools'
$readme = Join-Path $repo 'packaging/TRACK_TOOLS.txt'

# Pin installed files as well as downloads: a warm build verifies everything
# offline, and missing or damaged files can be repaired from the download cache.
$files = [ordered]@{
    'xdelta3.exe' = 'a86251ad67ca4972ea6423612fa154a35eccd6c5c2e9ad847b8a8afc2d9d53ac'
    'bigtool.exe' = 'f14cfa7f22ad4fae937363ab9d59c6e90f31e2f16c76358298227002dc54e02b'
    'CTRFramework.dll' = 'b66a5c7ee7f5e2be4d7880566028f5fb07b4246b396e7e3b2a662fcb23221fb4'
    'NAudio.dll' = 'bc4bacc3b8b28d898f1671b79f216cca439f95eb60cd32d3e3ecafbecac42780'
    'Newtonsoft.Json.dll' = 'b624949df8b0e3a6153fdfb730a7c6f4990b6592ee0d922e1788433d276610f3'
    'xdelta-LICENSE.txt' = '6cf63d87586c7c25c3ab8e62eef5c75bbaa982b0a6f9c00c59e2720255aac9ec'
    'CTR-tools-LICENSE.txt' = 'ab3fe34d3c62e3348015a4dd0128b09a5da2024d1467a478311003373c18a3f0'
    'NAudio-LICENSE.txt' = '0027e744e6750bc02408b90c6909a7a33093477083d59a7e2660477e05307689'
    'Newtonsoft.Json-LICENSE.txt' = 'd7f83bbea3264a8e23d7739e44d9db1a4994b357274df6f00876eb2e623827ad'
    'Kaitai-LICENSE.txt' = 'fb9ff6ffd086d8615ab816b9aac1ac6a0eaf744a53b24a9922cc0e2de30b688e'
    'PlyImporter-LICENSE.txt' = '2d0c505af17bcf3eef26f91e20c50d1ec43dd6bc49c879659b696945190c162f'
    'sources/CTR-tools-r14.zip' = '34864d88579bd7ce4b2f12788838951098bfedc3928293805e15ddb19336100d'
    'README.txt' = (Get-FileHash -LiteralPath $readme -Algorithm SHA256).Hash
}

function Test-VerifiedFile([string] $Path, [string] $Hash) {
    return (Test-Path -LiteralPath $Path -PathType Leaf) -and
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -eq $Hash
}

function Test-Installation([string] $Directory) {
    foreach ($entry in $files.GetEnumerator()) {
        if (-not (Test-VerifiedFile (Join-Path $Directory $entry.Key) $entry.Value)) { return $false }
    }
    return $true
}

if (Test-Installation $tools) {
    Write-Host 'Track tools verified: xdelta3 3.2.2 and CTR-tools r14 (no download needed)'
    return
}
if ($VerifyOnly) { throw "Track tools are missing or invalid in $tools. Build the Windows game first." }

$scratch = Join-Path ([IO.Path]::GetTempPath()) ('ctr-track-tools-' + [guid]::NewGuid())

function Get-VerifiedDownload([string] $Url, [string] $Hash, [string] $Name) {
    $cached = Join-Path $Cache $Name
    if (Test-VerifiedFile $cached $Hash) { return $cached }
    $temporary = Join-Path $scratch $Name
    Write-Host "Downloading $Name"
    Invoke-WebRequest $Url -OutFile $temporary
    if (-not (Test-VerifiedFile $temporary $Hash)) { throw "Checksum mismatch: $Name" }
    Move-Item -LiteralPath $temporary -Destination $cached -Force
    return $cached
}

try {
    New-Item -ItemType Directory -Path $scratch -Force | Out-Null
    New-Item -ItemType Directory -Path $Cache -Force | Out-Null
    $xdeltaZip = Get-VerifiedDownload `
        'https://github.com/jmacd/xdelta/releases/download/v3.2.2/xdelta3-3.2.2-windows-x86.zip' `
        '42429a6719795bb486e91d8928d5bbdba2cead8b2bc8515f812d3cf2f7b3dd55' 'xdelta3-3.2.2-windows-x86.zip'
    Expand-Archive -LiteralPath $xdeltaZip -DestinationPath (Join-Path $scratch 'xdelta')
    $ctrZip = Get-VerifiedDownload `
        'https://github.com/CTR-tools/CTR-tools/releases/download/r14/ctr-tools-x86-r14.zip' `
        '9036daabddccf6df78b1acca087e679e435fe28d119803d657b287326ee0121b' 'ctr-tools-x86-r14.zip'
    Expand-Archive -LiteralPath $ctrZip -DestinationPath (Join-Path $scratch 'ctr-tools')
    $prepared = Join-Path $scratch 'prepared'
    New-Item -ItemType Directory -Path $prepared -Force | Out-Null
    $decoder = Get-ChildItem (Join-Path $scratch 'xdelta') -Filter 'xdelta3.exe' -Recurse | Select-Object -First 1
    if ($null -eq $decoder) { throw 'xdelta3.exe not found in release archive' }
    Copy-Item -LiteralPath $decoder.FullName -Destination (Join-Path $prepared 'xdelta3.exe')
    $ctrRoot = Join-Path $scratch 'ctr-tools/ctr-tools-x86-r14'
    foreach ($name in @('bigtool.exe', 'CTRFramework.dll', 'NAudio.dll', 'Newtonsoft.Json.dll')) {
        Copy-Item -LiteralPath (Join-Path $ctrRoot $name) -Destination (Join-Path $prepared $name)
    }
    $sourceZip = Get-VerifiedDownload 'https://codeload.github.com/CTR-tools/CTR-tools/zip/refs/tags/r14' `
        $files['sources/CTR-tools-r14.zip'] 'CTR-tools-r14-source.zip'
    $sources = Join-Path $prepared 'sources'
    New-Item -ItemType Directory -Path $sources -Force | Out-Null
    Copy-Item -LiteralPath $sourceZip -Destination (Join-Path $sources 'CTR-tools-r14.zip')
    Expand-Archive -LiteralPath $sourceZip -DestinationPath (Join-Path $scratch 'source')
    $sourceRoot = Join-Path $scratch 'source/CTR-tools-r14'
    Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination (Join-Path $prepared 'CTR-tools-LICENSE.txt')
    foreach ($name in @('Kaitai', 'PlyImporter')) {
        Copy-Item -LiteralPath (Join-Path $sourceRoot "ctr-tools/CTRFramework/ThirdParty/$name/LICENSE") `
            -Destination (Join-Path $prepared "$name-LICENSE.txt")
    }
    $licenses = [ordered]@{
        'xdelta-LICENSE.txt' = 'https://raw.githubusercontent.com/jmacd/xdelta/v3.2.2/xdelta3/LICENSE'
        'NAudio-LICENSE.txt' = 'https://raw.githubusercontent.com/naudio/NAudio/v1.10.0/license.txt'
        'Newtonsoft.Json-LICENSE.txt' = 'https://raw.githubusercontent.com/JamesNK/Newtonsoft.Json/13.0.1/LICENSE.md'
    }
    foreach ($entry in $licenses.GetEnumerator()) {
        $license = Get-VerifiedDownload $entry.Value $files[$entry.Key] $entry.Key
        Copy-Item -LiteralPath $license -Destination (Join-Path $prepared $entry.Key)
    }
    Copy-Item -LiteralPath $readme -Destination (Join-Path $prepared 'README.txt')
    if (-not (Test-Installation $prepared)) { throw 'Prepared track tools failed verification' }
    foreach ($entry in $files.GetEnumerator()) {
        $target = Join-Path $tools $entry.Key
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $prepared $entry.Key) -Destination $target -Force
    }
    Write-Host "Installed xdelta3 3.2.2 and CTR-tools r14 in $tools"
}
finally {
    if (Test-Path -LiteralPath $scratch) { Remove-Item -LiteralPath $scratch -Recurse -Force }
}
