<#
.SYNOPSIS
    The cache-key and object-reuse phases for the SDL build tree in CI.

.DESCRIPTION
    Two phases that straddle the cache restore, so they cannot sit in one stretch
    of workflow:

      ComputeKey   runs before the restore and publishes the cache key.

      MarkCurrent  runs after the restore and makes the restored objects look
                   newer than the sources, so make reuses them instead of
                   rebuilding SDL from scratch.

    Both phases live in one file because the second is only safe because of the
    first. MarkCurrent presents cached objects to make as up to date, so a key
    that ever let through objects built from different SDL sources would silently
    compile the wrong library. Keeping them together keeps that dependency
    visible.

.PARAMETER Mode
    Which phase to run.

.PARAMETER CachePath
    The SDL build tree, relative to the repository root unless rooted.

.EXAMPLE
    ./scripts/sdl_cache.ps1 -Mode ComputeKey

    Prints the cache key and, under GitHub Actions, publishes it as step outputs.

.EXAMPLE
    ./scripts/sdl_cache.ps1 -Mode MarkCurrent

    Marks the restored SDL build tree as current. Does nothing if the tree is
    absent, which is what a cache miss looks like.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('ComputeKey', 'MarkCurrent')]
    [string] $Mode,

    [string] $CachePath = 'build\externals\SDL'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Resolve-CachePath {
    <#
    .SYNOPSIS
        Makes the cache path absolute, so the working directory does not matter.
    #>
    if ([System.IO.Path]::IsPathRooted($CachePath)) {
        return $CachePath
    }
    return Join-Path $RepoRoot $CachePath
}

function Invoke-ComputeKey {
    <#
    .SYNOPSIS
        Publishes a cache key pinning every input that can change the objects.
    #>
    $versionHeaderPath = Join-Path $RepoRoot 'externals\SDL\include\SDL3\SDL_version.h'
    $versionHeader = Get-Content -LiteralPath $versionHeaderPath -Raw

    $version = ([regex]::Matches($versionHeader, 'SDL_(?:MAJOR|MINOR|MICRO)_VERSION\s+(\d+)') |
        ForEach-Object { $_.Groups[1].Value }) -join '.'
    if (-not $version) {
        throw "No SDL version macros found in $versionHeaderPath"
    }

    # The vendored sources, and the root CMakeLists.txt because that is where the
    # SDL_* options handed to add_subdirectory are set.
    Push-Location $RepoRoot
    try {
        $sources = (git rev-parse 'HEAD:externals/SDL')
        $buildOptions = (git rev-parse 'HEAD:CMakeLists.txt')
        $gitExit = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }
    if ($gitExit -ne 0) {
        throw "git rev-parse failed; is this a checkout with a full history?"
    }

    $parts = @(
        'windows-i686-release'
        $version
        $sources.Trim()
        $buildOptions.Trim()
        # setup-msys2 updates its packages on every run, so a compiler upgrade has
        # to invalidate this cache rather than quietly reuse objects that the
        # previous compiler produced.
        ((i686-w64-mingw32-gcc --version) | Select-Object -First 1)
        ((cmake --version) | Select-Object -First 1)
    )

    $sha = [System.Security.Cryptography.SHA256]::Create()
    $digest = $sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes(($parts -join "`n")))
    $hash = ([System.BitConverter]::ToString($digest)).Replace('-', '').ToLowerInvariant().Substring(0, 12)

    Write-Host "SDL $version, cache key hash $hash"
    $parts | ForEach-Object { Write-Host "  key part: $_" }

    # Under Actions the caller needs these as step outputs. Run by hand there is
    # nothing to publish to, and the printed values are the result.
    if ($env:GITHUB_OUTPUT) {
        Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "version=$version"
        Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "hash=$hash"
    }
}

function Invoke-MarkCurrent {
    <#
    .SYNOPSIS
        Presents restored objects to make as up to date.
    #>
    $sdlBuild = Resolve-CachePath
    if (-not (Test-Path -LiteralPath $sdlBuild)) {
        Write-Host "No SDL build tree at $sdlBuild; SDL will be compiled."
        return
    }

    # actions/cache restores the timestamps archived alongside the build, which
    # are older than the checkout that has just happened. Left as they are, make
    # judges every source newer and rebuilds SDL from scratch, which is the one
    # thing this cache exists to prevent. ComputeKey has already established that
    # the sources behind these objects are unchanged, so presenting them as up to
    # date is safe.
    $files = @(Get-ChildItem -LiteralPath $sdlBuild -Recurse -File -Force)
    $now = Get-Date
    $files | ForEach-Object { $_.LastWriteTime = $now }
    Write-Host "Marked $($files.Count) SDL build files as current."
}

switch ($Mode) {
    'ComputeKey' { Invoke-ComputeKey }
    'MarkCurrent' { Invoke-MarkCurrent }
}
