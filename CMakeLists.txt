# file: CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(FnFinder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(FNFINDER_USE_SYSTEM_CAPSTONE "Use an installed Capstone via find_package" OFF)

set(FNFINDER_CAPSTONE_TARGET "")

# Options applied before Capstone is added to the build.
macro(fnfinder_configure_capstone_options)
  set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CAPSTONE_BUILD_MACOS_THIN OFF CACHE BOOL "" FORCE)
  # Keep the default architecture set (all arches) so AArch64 is always built,
  # regardless of the Capstone version's option naming.
endmacro()

if(FNFINDER_USE_SYSTEM_CAPSTONE)
  find_package(capstone CONFIG REQUIRED)
  if(TARGET capstone::capstone)
    set(FNFINDER_CAPSTONE_TARGET capstone::capstone)
  elseif(TARGET capstone)
    set(FNFINDER_CAPSTONE_TARGET capstone)
  endif()
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/capstone/CMakeLists.txt")
  fnfinder_configure_capstone_options()
  add_subdirectory(external/capstone EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  fnfinder_configure_capstone_options()
  FetchContent_Declare(
    capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
    GIT_TAG 5.0.1)
  FetchContent_MakeAvailable(capstone)
endif()

# Normalise the Capstone target name across versions.
if(NOT FNFINDER_CAPSTONE_TARGET)
  if(TARGET capstone::capstone)
    set(FNFINDER_CAPSTONE_TARGET capstone::capstone)
  elseif(TARGET capstone)
    set(FNFINDER_CAPSTONE_TARGET capstone)
  elseif(TARGET capstone-static)
    set(FNFINDER_CAPSTONE_TARGET capstone-static)
  else()
    message(FATAL_ERROR "FnFinder: could not locate a Capstone library target.")
  endif()
endif()

# ---------------------------------------------------------------------------
# Core library: everything except the CLI entry point.
# ---------------------------------------------------------------------------
set(FNFINDER_SOURCES
  src/common/logger.cc
  src/common/byte_reader.cc
  src/common/string_util.cc
  src/elf/elf_image.cc
  src/elf/elf_parser.cc
  src/symbol/itanium_demangler.cc
  src/symbol/name_resolver.cc
  src/disasm/aarch64/aarch64_mnemonics.cc
  src/disasm/capstone_disassembler.cc
  src/analysis/code_map.cc
  src/analysis/eh_frame_parser.cc
  src/analysis/recursive_disassembler.cc
  src/analysis/control_flow/code_patterns.cc
  src/analysis/control_flow/instruction_cache.cc
  src/analysis/control_flow/control_flow_graph.cc
  src/analysis/control_flow/cfg_builder.cc
  src/analysis/control_flow/function_extent_analyzer.cc
  src/analysis/function_analyzer.cc
  src/app/result_printer.cc)

add_library(fnfinder_core STATIC ${FNFINDER_SOURCES})
target_include_directories(fnfinder_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(fnfinder_core PUBLIC ${FNFINDER_CAPSTONE_TARGET})

if(MSVC)
  target_compile_options(fnfinder_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(fnfinder_core PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# CLI executable.
# ---------------------------------------------------------------------------
add_executable(FnFinder src/main.cc)
target_link_libraries(FnFinder PRIVATE fnfinder_core)

# On MinGW, statically link the C++/GCC runtimes so the executable does not pick
# up an incompatible libstdc++/libgcc DLL from PATH at runtime.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND WIN32)
  target_link_options(FnFinder PRIVATE -static -static-libgcc -static-libstdc++)
endif()
