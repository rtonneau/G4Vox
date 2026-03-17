# build.ps1  — place it at the root of G4Vox
<#
.SYNOPSIS
    Builds and installs the G4Vox library.

.DESCRIPTION
    Wraps the three CMake steps (configure / build / install) for G4Vox.
    Defaults target a RelWithDebInfo build against Geant4 v11.3.2.

.PARAMETER Config
    CMake build configuration. Default: RelWithDebInfo
    Allowed: Debug | Release | RelWithDebInfo | MinSizeRel

.PARAMETER Geant4Dir
    Path to the Geant4 CMake config directory.
    Default: C:\DEV\GEANT4\geant4-v11.3.2-install\lib\cmake\Geant4

.PARAMETER InstallPrefix
    Where to install the built library.
    Default: C:\DEV\GEANT4\LIB\G4Vox-install

.PARAMETER Clean
    Switch. If set, deletes the build directory before configuring.

.EXAMPLE
    .\build.ps1
    Normal build with all defaults.

.EXAMPLE
    .\build.ps1 -Config Release -Clean
    Clean rebuild in Release mode.

.EXAMPLE
    .\build.ps1 -InstallPrefix "C:\MyProject\deps"
    Install into a custom path.
#>
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "RelWithDebInfo",
    [string]$Geant4Dir = "",   # resolved below
    [string]$InstallPrefix = "",   # resolved below
    [switch]$Clean
)

# Resolve defaults from G4_ROOT if not explicitly passed
if (-not $Geant4Dir) { $Geant4Dir = "$env:G4_ROOT\lib\cmake\Geant4" }
if (-not $InstallPrefix) { $InstallPrefix = "$env:G4_ROOT\..\LIB\G4Vox-install" }

$BuildDir = "$PSScriptRoot\build"

# Optional clean rebuild
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

# ── Configure ─────────────────────────────────────────────────
Write-Host "`n[1/3] Configuring..." -ForegroundColor Cyan
cmake .. `
    -G "Visual Studio 18 2026" `
    -A x64 `
    -DGeant4_DIR="$Geant4Dir" `
    -DCMAKE_INSTALL_PREFIX="$InstallPrefix" `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

if ($LASTEXITCODE -ne 0) { Write-Error "Configure failed"; exit 1 }

# ── Build ──────────────────────────────────────────────────────
Write-Host "`n[2/3] Building..." -ForegroundColor Cyan
cmake --build . --config $Config --parallel

if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

# ── Install ────────────────────────────────────────────────────
Write-Host "`n[3/3] Installing..." -ForegroundColor Cyan
cmake --install . --config $Config

if ($LASTEXITCODE -ne 0) { Write-Error "Install failed"; exit 1 }

Write-Host "`nDone! Installed to: $InstallPrefix" -ForegroundColor Green

Set-Location $PSScriptRoot