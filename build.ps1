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

# Resolve G4_ROOT from process first, then user/machine env vars.
$G4Root = $env:G4_ROOT
if (-not $G4Root) { $G4Root = [Environment]::GetEnvironmentVariable("G4_ROOT", "User") }
if (-not $G4Root) { $G4Root = [Environment]::GetEnvironmentVariable("G4_ROOT", "Machine") }

# Resolve defaults from G4_ROOT if not explicitly passed
if (-not $Geant4Dir) {
    if (-not $G4Root) {
        throw "G4_ROOT is not set (process/user/machine) and -Geant4Dir was not provided."
    }

    $Geant4Candidates = @(
        "$G4Root\lib\cmake\Geant4",
        "$G4Root\lib64\cmake\Geant4",
        "$G4Root\cmake\Geant4"
    )

    foreach ($Candidate in $Geant4Candidates) {
        if (Test-Path (Join-Path $Candidate "Geant4Config.cmake")) {
            $Geant4Dir = $Candidate
            break
        }
    }

    if (-not $Geant4Dir) {
        throw "Could not locate Geant4Config.cmake under G4_ROOT='$G4Root'. Use -Geant4Dir to set it explicitly."
    }
}

if (-not $InstallPrefix) {
    if (-not $G4Root) {
        throw "G4_ROOT is not set (process/user/machine) and -InstallPrefix was not provided."
    }
    $InstallPrefix = "$G4Root\..\LIB\G4Vox-install"
}

$BuildDir = "$PSScriptRoot\build"
$VcpkgRoot = $env:VCPKG_ROOT
if (-not $VcpkgRoot) { $VcpkgRoot = [Environment]::GetEnvironmentVariable("VCPKG_ROOT", "User") }
if (-not $VcpkgRoot) { $VcpkgRoot = [Environment]::GetEnvironmentVariable("VCPKG_ROOT", "Machine") }
if (-not $VcpkgRoot) {
    throw "VCPKG_ROOT is not set (process/user/machine)."
}
$VcpkgRoot = $VcpkgRoot -replace '\\', '/'
$VcpkgToolchain = "$VcpkgRoot/scripts/buildsystems/vcpkg.cmake"
if (-not (Test-Path $VcpkgToolchain)) {
    throw "Invalid VCPKG_ROOT='$VcpkgRoot': missing toolchain file at '$VcpkgToolchain'."
}

$VcpkgInstalledDir = "$VcpkgRoot/installed"
if (-not (Test-Path $VcpkgInstalledDir)) {
    throw "Invalid VCPKG_ROOT='$VcpkgRoot': missing installed dir at '$VcpkgInstalledDir'."
}

$VcpkgTriplet = $env:VCPKG_DEFAULT_TRIPLET
if (-not $VcpkgTriplet) { $VcpkgTriplet = "x64-windows" }

$Hdf5Root = "$VcpkgInstalledDir/$VcpkgTriplet"
$Hdf5IncludeDir = "$Hdf5Root/include"
$Hdf5ConfigDir = "$Hdf5Root/share/hdf5"

if (-not (Test-Path "$Hdf5IncludeDir/H5pubconf.h")) {
    throw "HDF5 header not found at '$Hdf5IncludeDir/H5pubconf.h'. Check VCPKG_ROOT and triplet '$VcpkgTriplet'."
}

if (-not (Test-Path "$Hdf5ConfigDir/hdf5-config.cmake")) {
    throw "HDF5 config not found at '$Hdf5ConfigDir/hdf5-config.cmake'. Check VCPKG_ROOT and triplet '$VcpkgTriplet'."
}

$env:VCPKG_ROOT = $VcpkgRoot
if ($env:VCPKG_INSTALLED_DIR) {
    $env:VCPKG_INSTALLED_DIR = ($env:VCPKG_INSTALLED_DIR -replace '\\', '/')
}

# Optional clean rebuild
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

try {
    $PreferredGenerators = @(
        "Visual Studio 18 2026",
        "Visual Studio 17 2022"
    )

    $CMakeHelp = cmake --help 2>&1 | Out-String
    $Generator = $null
    foreach ($Candidate in $PreferredGenerators) {
        if ($CMakeHelp -match [regex]::Escape($Candidate)) {
            $Generator = $Candidate
            break
        }
    }

    if (-not $Generator) {
        throw "No supported Visual Studio generator found. Expected one of: Visual Studio 18 2026, Visual Studio 17 2022"
    }

    Write-Host "Using Geant4_DIR: $Geant4Dir" -ForegroundColor Cyan
    Write-Host "Using VCPKG_ROOT: $VcpkgRoot" -ForegroundColor Cyan
    Write-Host "Using VCPKG triplet: $VcpkgTriplet" -ForegroundColor Cyan
    Write-Host "Using HDF5 include: $Hdf5IncludeDir" -ForegroundColor Cyan
    Write-Host "Using CMake generator: $Generator" -ForegroundColor Cyan

    # ── Configure ─────────────────────────────────────────────────
    function Show-HDF5Diagnostics {
        $CacheFile = "$BuildDir\CMakeCache.txt"
        $ConfigureLog = "$BuildDir\CMakeFiles\CMakeConfigureLog.yaml"

        Write-Host "`n[diag] CMake HDF5/Geant4 diagnostics" -ForegroundColor Yellow

        if (Test-Path $CacheFile) {
            Write-Host "[diag] From CMakeCache.txt:" -ForegroundColor Yellow
            $Patterns = @(
                '^HDF5_',
                '^Geant4_',
                '^CMAKE_PREFIX_PATH:',
                '^CMAKE_TOOLCHAIN_FILE:',
                '^VCPKG_'
            )

            foreach ($Pattern in $Patterns) {
                Select-String -Path $CacheFile -Pattern $Pattern | ForEach-Object {
                    Write-Host ("  " + $_.Line)
                }
            }
        }
        else {
            Write-Host "[diag] CMakeCache.txt not found." -ForegroundColor Yellow
        }

        if (Test-Path $ConfigureLog) {
            Write-Host "[diag] From CMakeConfigureLog.yaml (H5 probe excerpt):" -ForegroundColor Yellow
            Select-String -Path $ConfigureLog -Pattern 'Looking for H5_HAVE_THREADSAFE|H5pubconf.h|HDF5_INCLUDE_DIRS|GEANT4_HAVE_H5_HAVE_THREADSAFE' | ForEach-Object {
                Write-Host ("  " + $_.Line.Trim())
            }
        }
        else {
            Write-Host "[diag] CMakeConfigureLog.yaml not found." -ForegroundColor Yellow
        }
    }

    function Invoke-Configure {
        cmake .. `
            -G "$Generator" `
            -A x64 `
            -DGeant4_DIR="$Geant4Dir" `
            -DCMAKE_INSTALL_PREFIX="$InstallPrefix" `
            -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON `
            -DHDF5_DIR="$Hdf5ConfigDir" `
            -DHDF5_ROOT="$Hdf5Root" `
            -DHDF5_INCLUDE_DIRS="$Hdf5IncludeDir" `
            -DHDF5_C_INCLUDE_DIRS="$Hdf5IncludeDir" `
            -DHDF5_CXX_INCLUDE_DIRS="$Hdf5IncludeDir" `
            -DVCPKG_INSTALLED_DIR="$VcpkgInstalledDir" `
            -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" `
            -Wno-dev
    }
    Write-Host "`n[1/3] Configuring..." -ForegroundColor Cyan
    Invoke-Configure
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`nConfigure failed. This may be due to a CMake cache mismatch." -ForegroundColor Yellow
        Show-HDF5Diagnostics
        $answer = Read-Host "Wipe CMakeCache.txt + CMakeFiles and retry? [y/N]"

        if ($answer -match '^[Yy]$') {
            Write-Host "Wiping cache..." -ForegroundColor Yellow
            Remove-Item -Force -ErrorAction SilentlyContinue "$BuildDir\CMakeCache.txt"
            Remove-Item -Recurse -Force -ErrorAction SilentlyContinue "$BuildDir\CMakeFiles"

            Invoke-Configure

            if ($LASTEXITCODE -ne 0) {
                Show-HDF5Diagnostics
                throw "Configure failed even after cache wipe"
            }
        }
        else {
            throw "Configure failed — cache wipe declined by user"
        }
    }


    # ── Build ──────────────────────────────────────────────────────
    Write-Host "`n[2/3] Building..." -ForegroundColor Cyan
    cmake --build . --config $Config --parallel

    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    # ── Install ────────────────────────────────────────────────────
    Write-Host "`n[3/3] Installing..." -ForegroundColor Cyan
    cmake --install . --config $Config

    if ($LASTEXITCODE -ne 0) { throw "Install failed" }

    Write-Host "`nDone! Installed to: $InstallPrefix" -ForegroundColor Green
}
catch {
    Write-Error $_.Exception.Message
    exit 1
}
finally {
    # Always runs — success OR failure
    Set-Location $PSScriptRoot
}
