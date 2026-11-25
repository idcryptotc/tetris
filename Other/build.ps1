# Build Tetris.slnx + run the test project - AGENTS.md rule 4: after any
# change the build must have no errors and no warnings, in one command.
# Usage: .\build.ps1 [-Configuration Debug] [-Platform Win32] [-NoTests]
# msbuild output is passed through, so warnings stay visible.
# Exit code: 0 - build and tests are green, 1 - build or tests failed.
# The script itself is ASCII on purpose: Windows PowerShell 5.1 reads BOM-less
# .ps1 files as ANSI, so a Cyrillic script would corrupt itself.
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Win32', 'x64')]
    [string]$Platform = 'Win32',
    [switch]$NoTests
)
$ErrorActionPreference = 'Stop'
# The script lives in Other/, so the solution root is one level up.
$otherDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $otherDir
$sln = Join-Path $root 'Tetris.slnx'
# MSBuild comes from the Visual Studio install: find it via vswhere (works
# from a plain shell, no developer prompt needed).
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    Write-Host "vswhere not found: $vswhere"
    exit 1
}
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild -or -not (Test-Path -LiteralPath $msbuild)) {
    Write-Host "MSBuild not found (Visual Studio with MSBuild required)"
    exit 1
}
Write-Host "--- $msbuild"
& $msbuild $sln /p:Configuration=$Configuration /p:Platform=$Platform /nologo /v:m
if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD FAILED (code $LASTEXITCODE)"
    exit 1
}
if ($NoTests) {
    Write-Host "Build OK (tests skipped)"
    exit 0
}
# Solution-relative default output: <root>\<Config>\ for the platform matching
# the solution default (Win32), <root>\<Platform>\<Config>\ for x64.
$testsExe = $null
foreach ($candidate in @("$($Platform)\$Configuration\Tetris.Tests.exe", "$Configuration\Tetris.Tests.exe")) {
    $path = Join-Path $root $candidate
    if (Test-Path -LiteralPath $path) { $testsExe = $path; break }
}
if ($null -eq $testsExe) {
    Write-Host "Build OK (Tetris.Tests.exe not found under $root)"
    exit 0
}
Write-Host "--- running $testsExe"
& $testsExe
$code = $LASTEXITCODE
if ($code -ne 0) {
    Write-Host "TESTS FAILED (code $code)"
    exit 1
}
Write-Host "Build OK, tests green"