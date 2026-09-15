# build.ps1 - build and optionally run "Quarter Turn".
#   .\build.ps1          build only
#   .\build.ps1 -Run     build then run
#   .\build.ps1 -Check   build and run the headless math check (PRD section 10)
#   .\build.ps1 -Release build with -O2 and NDEBUG (assertions off)
param([switch]$Run, [switch]$Check, [switch]$Release)

$ErrorActionPreference = "Stop"
$MinGW = "C:\msys64\mingw64\bin"
if (-not (Test-Path "$MinGW\g++.exe")) { throw "g++ not found at $MinGW" }
$env:PATH = "$MinGW;$env:PATH"

$root = $PSScriptRoot
$out  = Join-Path $root "build\quarterturn.exe"
$test = Join-Path $root "build\mathcheck.exe"
New-Item -ItemType Directory -Force (Join-Path $root "build") | Out-Null

$flags = @("-std=c++17", "-Wall", "-Wextra")
if ($Release) { $flags += @("-O2", "-DNDEBUG") } else { $flags += @("-O0", "-g") }

if ($Check) {
    Write-Host "building -> $test" -ForegroundColor Cyan
    & g++ -std=c++17 -Wall -Wextra -O2 (Join-Path $root "tests\mathcheck.cpp") -o $test
    if ($LASTEXITCODE -ne 0) { throw "test build failed ($LASTEXITCODE)" }
    & $test
    exit $LASTEXITCODE
}

$src  = @(Join-Path $root "src\main.cpp")
$libs = @("-lglew32", "-lfreeglut", "-lopengl32", "-lglu32")

Write-Host "building -> $out" -ForegroundColor Cyan
& g++ @flags @src -o $out @libs
if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }
Write-Host "ok" -ForegroundColor Green

if ($Run) { & $out }
