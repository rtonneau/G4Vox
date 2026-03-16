cmake_minimum_required(VERSION 3.16)

# ── Project declaration ───────────────────────────────────────────────────────
# VERSION is important: find_package(G4Vox 1.0 REQUIRED) will check it
project(G4Vox
    VERSION 0.1.0
    DESCRIPTION "Voxelization library for Geant4-based simulations"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Generate version header ───────────────────────────────────────────────────
configure_file(
    cmake/G4VoxVersion.hh.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/G4VoxVersion.hh
)

# ── Find dependencies ─────────────────────────────────────────────────────────
find_package(Geant4 REQUIRED)
#Look for toml++ library (installed by vcpkg)
find_package(tomlplusplus CONFIG REQUIRED)


# ── Collect sources ───────────────────────────────────────────────────────────
file(GLOB G4Vox_SOURCES
    src/*.cc
)

# ── Create the library target ─────────────────────────────────────────────────
# STATIC: the .a archive will be linked into the consumer's executable.
# The consumer does not need libG4Vox.so at runtime → easier deployment.
add_library(G4Vox STATIC ${G4Vox_SOURCES})

# ── Give the target a namespaced alias ────────────────────────────────────────
# This makes  target_link_libraries(myApp G4Vox::G4Vox)  work
# even when building G4Vox as a subdirectory (add_subdirectory use case).
add_library(G4Vox::G4Vox ALIAS G4Vox)

# ── Include directories ───────────────────────────────────────────────────────
# BUILD_INTERFACE  → path used when compiling G4Vox itself and
#                    when another project uses add_subdirectory()
# INSTALL_INTERFACE→ path used after cmake --install, relative to install prefix
#                    CMake will prepend the actual install prefix automatically.
target_include_directories(G4Vox
    PUBLIC
    # At build time  → full source path
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    # After install  → relative to install prefix
    $<INSTALL_INTERFACE:include>
)

# ── Link Geant4 ───────────────────────────────────────────────────────────────
# PUBLIC means: "I need Geant4, AND anyone linking G4Vox also needs Geant4."
# This is correct because G4Vox's PUBLIC headers expose G4 types.
# If G4 types only appeared in .cc files, PRIVATE would be enough.
target_link_libraries(G4Vox
    PUBLIC
    ${Geant4_LIBRARIES}
    PUBLIC
    tomlplusplus::tomlplusplus
)

# Also propagate Geant4 include dirs to consumers
target_include_directories(G4Vox
    PUBLIC
    ${Geant4_INCLUDE_DIRS} # Geant4 headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # G4Vox headers (build)
    $<INSTALL_INTERFACE:include> # G4Vox headers (install)
)
# ── Compiler options ──────────────────────────────────────────────────────────
# ❌ BEFORE — GCC flags fed to MSVC, breaks the build on Windows.
#target_compile_options(G4Vox PRIVATE -Wall -Wextra -Wpedantic)
# ✔ AFTER — compiler-agnostic, works on MSVC, GCC and Clang
target_compile_options(G4Vox PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# ═════════════════════════════════════════════════════════════════════════════
#  INSTALLATION RULES
#  Everything below defines what "cmake --install ." actually does
# ═════════════════════════════════════════════════════════════════════════════

include(GNUInstallDirs)
# GNUInstallDirs defines standard variables:
#   CMAKE_INSTALL_LIBDIR     → lib   (or lib64 on some systems)
#   CMAKE_INSTALL_INCLUDEDIR → include
#   CMAKE_INSTALL_DATADIR    → share

# ── Install the library binary ────────────────────────────────────────────────
# EXPORT G4VoxTargets: registers this target in an export set.
# We will install that export set later so find_package() can import the target.
install(TARGETS G4Vox
    EXPORT G4VoxTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # .a file
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so file if SHARED
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # .dll on Windows
)

# ── Install the headers ───────────────────────────────────────────────────────
# The entire include/G4Vox/ folder goes to <prefix>/include/G4Vox/
install(DIRECTORY include/G4Vox/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/G4Vox
)

# ── Install the export set (the Targets file) ─────────────────────────────────
# This generates G4VoxTargets.cmake in the install tree.
# That file contains the IMPORTED target definition with correct paths.
install(EXPORT G4VoxTargets
    FILE G4VoxTargets.cmake
    NAMESPACE G4Vox:: # consumers use G4Vox::G4Vox
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/G4Vox
)

# ── Generate G4VoxConfigVersion.cmake ─────────────────────────────────────
# Allows  find_package(G4Vox 1.0 REQUIRED)  to check compatibility.
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/G4VoxConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion # 1.x.x is compatible with 1.0.0
)

# ── Generate G4VoxConfig.cmake from the template ───────────────────────────
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/G4VoxConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/G4VoxConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/G4Vox
)

# ── Install both generated cmake files ────────────────────────────────────────
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/G4VoxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/G4VoxConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/G4Vox
)
