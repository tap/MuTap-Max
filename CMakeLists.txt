# MuTap-Max — Max/MSP externals over the MuTap adaptive-filtering library.
# Structure follows the Cycling '74 Min-DevKit package convention (mirrors
# AmbiTap-Max).

cmake_minimum_required(VERSION 3.24)
project(MuTap-Max)

# Raise the default C++ standard for every target. min-api uses std::clamp
# (C++17) and the MuTap core needs C++20; AppleClang defaults high enough so
# macOS is fine, but MSVC defaults to C++14, which breaks on std::clamp.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# The Max SDK pulls in <windows.h>, whose `min`/`max` macros break min-api's
# `min::clamp` (and any std::min/std::max). Suppress them (and trim the heavy
# include) for every target added below, including the MuTap subdirectory.
if (WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    # min-devkit builds the externals with the static CRT (/MT). The MuTap
    # core's Ooura FFT target uses CMP0091-NEW, so it would otherwise default
    # to the dynamic /MD and clash (LNK4098). Force the static runtime on the
    # policy-NEW targets so every object matches /MT.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if (MSVC)
        # /bigobj: min-api-heavy TUs can blow past the COFF section limit.
        add_compile_options(/bigobj)
        # /FIalgorithm: min-api's c74_min_limit.h does `using std::clamp;` without
        #   including <algorithm>; it resolves transitively on AppleClang/libc++
        #   but not on MSVC's STL, so force-include it. C++ ONLY — force-including
        #   a C++ STL header into the C sources (Ooura FFT) trips
        #   "STL1003: expected C++ compiler".
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/FIalgorithm>)
    endif ()
endif ()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

if (APPLE)
    if (${CMAKE_GENERATOR} MATCHES "Xcode")
        if (${XCODE_VERSION} VERSION_LESS 10)
            message(STATUS "Xcode 10 or higher is required. Please install from the Mac App Store.")
            return ()
        elseif (${XCODE_VERSION} VERSION_GREATER_EQUAL 12)
            set(C74_BUILD_FAT YES)
        endif ()
    endif ()

    if (NOT CMAKE_OSX_ARCHITECTURES)
        if (C74_BUILD_FAT)
            set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
        else ()
            set(CMAKE_OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR} CACHE STRING "macOS architecture" FORCE)
        endif ()
        message("CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES}")
    endif ()
endif ()

# Min-API CMake helpers (SUBDIRLIST, package layout, pre/post target scripts).
#
# max-package.cmake only reads the version (into PACKAGE_VERSION) when a
# package-info.json.in *template* is present; this package ships a static
# package-info.json, so that block is skipped and PACKAGE_VERSION is left empty.
# On Windows that trips the verinfo step's string(REPLACE "." "," PACKAGE_VERSION
# ${PACKAGE_VERSION}) — "requires at least four arguments" — while macOS skips
# the WIN32-only verinfo entirely. Provide the version (matching
# package-info.json) so the Windows build configures.
set(PACKAGE_VERSION "0.0.1")
include(${CMAKE_CURRENT_SOURCE_DIR}/source/min-api/script/min-package.cmake)

# MuTap: the adaptive-filtering core, pinned as the submodules/MuTap
# submodule. Override with -DMuTap_ROOT=/path/to/MuTap to build against a
# sibling checkout during development. Header-only apart from the vendored
# Ooura FFT (MuTap::fft), which the `mutap` interface target carries.
set(MuTap_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/submodules/MuTap"
    CACHE PATH "Path to the MuTap library source tree")
if (NOT EXISTS "${MuTap_ROOT}/include/mutap/pem_afc.h")
    message(FATAL_ERROR "MuTap not found at ${MuTap_ROOT}. Set -DMuTap_ROOT.")
endif ()

# Build MuTap's targets so objects can pull in what they need:
#  - mutap        (interface target: headers + cxx_std_20 + MuTap::fft)
#  - MuTap::fft   (the vendored Ooura rdft the FDAF core calls)
# Tests are off here because MuTap is not the top-level project.
add_subdirectory(${MuTap_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/mutap EXCLUDE_FROM_ALL)

# Min-Lib, if present (header/static helpers shared across objects).
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/min-lib)
endif ()

# One external per folder under source/projects/.
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
    endif ()
endforeach ()
