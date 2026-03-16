# G4Vox

> **A Geant4 utility library for 3D voxel-based scoring**

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-green.svg)](https://github.com/rtonneau/G4Vox/releases/tag/v0.1.0)
[![Geant4](https://img.shields.io/badge/Geant4-11.3.2-red.svg)](https://geant4.web.cern.ch)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

---

## Overview

**G4Vox** is a lightweight C++ library built on top of [Geant4](https://geant4.web.cern.ch)
that automates the voxelization of `G4Box` volumes and provides a structured
framework for scoring and retrieving physical quantities resolved on a 3D voxel grid.

```
     G4Box geometry
           │
           ▼
┌──────────────────────┐
│       G4Vox          │
│  ┌──────────────┐    │
│  │  Voxelizer   │    │   ──►  NX × NY × NZ grid
│  └──────────────┘    │
│  ┌──────────────┐    │
│  │   G4VAccu    │    │   ──►  Dose / LET / ...
│  └──────────────┘    │
└──────────────────────┘
           │
           ▼
    3D scored data
    (retrievable per voxel)
```

### Key features

- 🔲 **Automatic voxelization** of any `G4Box` volume into a configurable NX × NY × NZ grid
- 🔌 **Clean API** designed to integrate seamlessly into any Geant4 application
- 🧩 **Modular design** — Design yourself the accumulator you need
- ⚡ **Compatible with Geant4 11.3+**

---

## Requirements

| Dependency | Minimum version | Notes              |
|------------|-----------------|--------------------|
| CMake      | 3.16            | Build system       |
| C++        | 17              | Required by Geant4 |
| Visual Studio 2026 (v18)     | with **Desktop C++ workload** |
| Geant4     | 11.0            | Must be built with `GEANT4_BUILD_MULTITHREADED=ON` for MT support |
| vcpkg      |                 | [github.com/microsoft/vcpkg](https://github.com/microsoft/vcpkg)  |

> Tomlplusplus is used to produce manifest file containing simulation information.

---

## Build

G4Vox uses a standard CMake workflow.

### 1 — Clone

```bash
  git clone https://github.com/YOUR_USERNAME/G4Vox.git
  cd G4Vox
---

### 2 — Configure

### Step 1 — Install tomlplusplus via vcpkg

Open a **Developer PowerShell** and run:

```powershell
vcpkg install tomlplusplus:x64-windows
```

Make sure your `VCPKG_ROOT` environment variable is set, e.g.:

```powershell
  $env:VCPKG_ROOT = "C:/vcpkg"
```

---

### Step 2 — Configure

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

### Step 3 — Build

```powershell
cmake --build . --config RelWithDebInfo
```

---

### Step 4 — Install

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

### Step 5 — Use G4Vox in a consumer project

#### 5.1 — Update `CMakeLists.txt`

```cmake
find_package(G4Vox REQUIRED)

target_link_libraries(your_target PRIVATE G4Vox::G4Vox)
```

#### 5.2 — Update VSCode `settings.json`

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
