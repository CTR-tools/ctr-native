<#
.SYNOPSIS
    Fingerprints the environment and configuration inputs for the CI CMake cache.

.DESCRIPTION
    Gameplay sources and the branch name are deliberately excluded. Reuse is
    exact-key only: cached probe results must not cross toolchain, SDL, runner
    image, checkout-path or build-configuration changes.
#>
[CmdletBinding()]
param(
    [string] $RepoRoot = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

function Invoke-CheckedCommand([string] $Command, [string[]] $Arguments) {
    $output = & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE."
    }
    return $output
}

$parts = [System.Collections.Generic.List[string]]::new()
$parts.Add('cmake-config-v1-windows-i686-ninja-release-tests-on')
$parts.Add("checkout=$RepoRoot")
$parts.Add("os=$([Environment]::OSVersion.VersionString)")

foreach ($name in @(
    'ImageOS', 'ImageVersion', 'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'CPPFLAGS',
    'LDFLAGS', 'CMAKE_GENERATOR', 'CMAKE_TOOLCHAIN_FILE', 'CMAKE_PREFIX_PATH',
    'CMAKE_LIBRARY_PATH', 'CMAKE_INCLUDE_PATH', 'PKG_CONFIG_PATH',
    'PKG_CONFIG_LIBDIR', 'CMAKE_C_COMPILER_LAUNCHER', 'CMAKE_CXX_COMPILER_LAUNCHER'
)) {
    $parts.Add("$name=$([Environment]::GetEnvironmentVariable($name))")
}

$cmakeVersion = ''
$compiler = ''
foreach ($tool in @('cmake', 'ninja', 'i686-w64-mingw32-gcc', 'c++', 'pkg-config', 'pkgconf', 'git')) {
    $command = Get-Command $tool -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $command -and $tool -in @('pkg-config', 'pkgconf')) {
        $parts.Add("tool=$tool|absent")
        continue
    }
    if (-not $command) {
        throw "Required tool not found: $tool"
    }
    $path = $command.Source
    $version = @(Invoke-CheckedCommand -Command $path -Arguments @('--version'))[0]
    $parts.Add("tool=$path|$version")
    if ($tool -eq 'cmake') {
        $cmakeVersion = $version -replace '^cmake version ', ''
    }
    if ($tool -eq 'i686-w64-mingw32-gcc') {
        $compiler = $path
    }
}
if ($cmakeVersion -notmatch '^\d+\.\d+\.\d+[-A-Za-z0-9.]*$') {
    throw "Unrecognized CMake version: $cmakeVersion"
}

# Include all installed MSYS2 packages: headers and libraries affect probes too.
$msysRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $compiler))
$pacman = Join-Path $msysRoot 'usr/bin/pacman.exe'
$packages = @(Invoke-CheckedCommand -Command $pacman -Arguments @('-Q')) | Sort-Object
foreach ($package in $packages) {
    $parts.Add("package=$package")
}

Push-Location $RepoRoot
try {
    $sdlTree = Invoke-CheckedCommand -Command 'git' -Arguments @('rev-parse', 'HEAD:externals/SDL')
    $parts.Add("sdl=$sdlTree")
    $tracked = @(Invoke-CheckedCommand -Command 'git' -Arguments @('ls-files'))
    $configuration = @($tracked | Where-Object {
        $_ -match '(^|/)CMakeLists\.txt$|\.cmake$|\.in$' -or
        $_ -in @('CMakePresets.json', 'build.bat', '.github/workflows/ci.yml')
    })
    # Explicitly include these new files even before they have been staged.
    $configuration += 'scripts/ci/cmake-cache-key.ps1', '.github/actions/cache-cmake/action.yml'
    foreach ($file in ($configuration | Sort-Object -Unique)) {
        $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
        $parts.Add("file=$file|$hash")
    }
}
finally {
    Pop-Location
}

$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $digest = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes(($parts -join "`n")))
}
finally {
    $sha.Dispose()
}
$hash = ([BitConverter]::ToString($digest)).Replace('-', '').ToLowerInvariant()
$key = "cmake-config-windows-i686-v1-$hash"
Write-Host "CMake configuration cache: $key"
Write-Host "CMake $cmakeVersion; $($packages.Count) MSYS2 packages; $($configuration.Count) configuration inputs."
if ($env:GITHUB_OUTPUT) {
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "key=$key" -Encoding utf8
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "cmake-version=$cmakeVersion" -Encoding utf8
}
