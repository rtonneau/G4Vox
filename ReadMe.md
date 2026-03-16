# G4Vox — Build & Install Guide

> Geant4 voxelization library wrapping `tomlplusplus` for TOML-based geometry configuration.

---

## Prerequisites

| Tool | Notes |
|------|-------|
| CMake ≥ 3.20 | [cmake.org](https://cmake.org) |
| Visual Studio 2026 (v18) | with **Desktop C++ workload** |
| Geant4 v11.3.2 (installed) | e.g. `C:/DEV/GEANT4/geant4-v11.3.2-install` |
| vcpkg | [github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg) |

---

## Step 1 — Install tomlplusplus via vcpkg

Open a **Developer PowerShell** and run:

```powershell
vcpkg install tomlplusplus:x64-windows
```

Make sure your `VCPKG_ROOT` environment variable is set, e.g.:

```powershell
$env:VCPKG_ROOT = "C:/vcpkg"
```

---

## Step 2 — Configure

From the `G4Vox` source directory:

```powershell
mkdir build
cd build

cmake .. `
  -G "Visual Studio 18 2026" `
  -A x64 `
  -DGeant4_DIR="C:\DEV\GEANT4\geant4-v11.3.2-install\lib\cmake\Geant4" `
  -DCMAKE_INSTALL_PREFIX="C:\DEV\GEANT4\LIB\G4Vox-install" `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

> **Note:** Adjust `Geant4_DIR` and `CMAKE_INSTALL_PREFIX` to match your local paths.

---

## Step 3 — Build

```powershell
cmake --build . --config RelWithDebInfo
```

---

## Step 4 — Install

```powershell
cmake --install . --config RelWithDebInfo
```

The library will be installed under `C:/DEV/GEANT4/LIB/G4Vox-install` (or your chosen prefix):

```
G4Vox-install/
  ├── include/
  ├── lib/
  │   └── cmake/G4Vox/
  │       ├── G4VoxConfig.cmake
  │       ├── G4VoxConfigVersion.cmake
  │       └── G4VoxTargets.cmake
  └── bin/
```

---

## Step 5 — Use G4Vox in a consumer project

### 5.1 — Update `CMakeLists.txt`

```cmake
find_package(G4Vox REQUIRED)

target_link_libraries(your_target PRIVATE G4Vox::G4Vox)
```

### 5.2 — Update VSCode `settings.json`

```jsonc
{
  "cmake.configureArgs": [
    "-DCMAKE_PREFIX_PATH=${env:G4_ROOT};C:/DEV/GEANT4/LIB/G4Vox-install",
    "-DCMAKE_TOOLCHAIN_FILE=${env:VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  ]
}
```

> Replace `C:/DEV/GEANT4/LIB/G4Vox-install` with your actual G4Vox install prefix if different.  
> `G4_ROOT` should point to your Geant4 install root, e.g. `C:/DEV/GEANT4/geant4-v11.3.2-install`.

---

## Troubleshooting

### `tomlplusplus::tomlplusplus` target not found

Add `find_dependency(tomlplusplus REQUIRED)` in `cmake/G4VoxConfig.cmake.in` **before** the `include(G4VoxTargets.cmake)` line, then rebuild and reinstall.

### LNK2005 — multiply defined symbols

Method definitions must live in `.cc` files, **not** in headers.  
Include guards (`#pragma once`) do not prevent linker duplicates across translation units.

---

## Environment Variables Summary

| Variable | Example value |
|----------|--------------|
| `G4_ROOT` | `C:/DEV/GEANT4/geant4-v11.3.2-install` |
| `VCPKG_ROOT` | `C:/vcpkg` |
