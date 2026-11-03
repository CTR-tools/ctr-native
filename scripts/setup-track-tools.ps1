<# Installs the local xdelta import tools. No game data or patches are downloaded. #>
[CmdletBinding()]
param([string] $Assets = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($Assets -eq '') { $Assets = Join-Path (Split-Path -Parent $PSScriptRoot) 'assets' }
$tools = Join-Path $Assets 'tools'
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('ctr-track-tools-' + [guid]::NewGuid())

function Get-VerifiedArchive([string] $Url, [string] $Hash, [string] $Name) {
    $archive = Join-Path $scratch ($Name + '.zip')
    Invoke-WebRequest $Url -OutFile $archive
    if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $Hash) {
        throw "Checksum mismatch: $Name"
    }
    $destination = Join-Path $scratch $Name
    Expand-Archive $archive $destination
    return $destination
}

try {
    New-Item -ItemType Directory -Path $scratch | Out-Null
    $xdelta = Get-VerifiedArchive `
        'https://github.com/jmacd/xdelta/releases/download/v3.2.2/xdelta3-3.2.2-windows-x86.zip' `
        '42429a6719795bb486e91d8928d5bbdba2cead8b2bc8515f812d3cf2f7b3dd55' 'xdelta'
    $ctr = Get-VerifiedArchive `
        'https://github.com/CTR-tools/CTR-tools/releases/download/r14/ctr-tools-x86-r14.zip' `
        '9036daabddccf6df78b1acca087e679e435fe28d119803d657b287326ee0121b' 'ctr-tools'
    New-Item -ItemType Directory -Path $tools -Force | Out-Null
    $decoder = Get-ChildItem $xdelta -Filter 'xdelta3.exe' -Recurse | Select-Object -First 1
    if ($null -eq $decoder) { throw 'xdelta3.exe not found in release archive' }
    Copy-Item $decoder.FullName (Join-Path $tools 'xdelta3.exe')
    $ctrRoot = Join-Path $ctr 'ctr-tools-x86-r14'
    foreach ($file in @('bigtool.exe', 'CTRFramework.dll', 'NAudio.dll', 'Newtonsoft.Json.dll')) {
        Copy-Item (Join-Path $ctrRoot $file) (Join-Path $tools $file)
    }
    Invoke-WebRequest 'https://raw.githubusercontent.com/jmacd/xdelta/v3.2.2/xdelta3/LICENSE' `
        -OutFile (Join-Path $tools 'xdelta-LICENSE.txt')
    Invoke-WebRequest 'https://raw.githubusercontent.com/CTR-tools/CTR-tools/r14/LICENSE' `
        -OutFile (Join-Path $tools 'CTR-tools-LICENSE.txt')
    Write-Host "Installed xdelta3 3.2.2 and CTR-tools r14 in $tools"
}
finally {
    if (Test-Path $scratch) { Remove-Item $scratch -Recurse -Force }
}
