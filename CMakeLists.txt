# Modern CMake
cmake_minimum_required(VERSION 3.18)

project(VTIL-Utils)

# Visual Studio generator specific flags
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # HACK: DO NOT this to add compiler flags/definitions, use target_compile_options on a
    # target instead https://cmake.org/cmake/help/latest/command/target_compile_options.html

    # Enable multiprocessor compilation
    add_compile_options(/MP)
endif()

# Add module path for dependencies
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Enable solution folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Workaround for https://github.com/cpm-cmake/CPM.cmake/issues/281
set(CPM_SOURCE_CACHE OFF CACHE BOOL "" FORCE)

# Hack for working Debug configuration
add_definitions(-DVTIL_SYMEX_VERIFY=0)

# Dependency packages
set(CMAKE_FOLDER "Dependencies")
include(CPM)
include(VTIL-Core)
include(VTIL-NativeLifters)
include(args)
include(asmjit)

# Target to generate third_party.txt
set(DEPENDENCY_PACKAGES "${CPM_PACKAGES}")
include(CPMLicenses)
cpm_licenses_create_disclaimer_target(
    write-licenses "${CMAKE_CURRENT_BINARY_DIR}/third_party.txt" "${DEPENDENCY_PACKAGES}"
)

set(CMAKE_FOLDER "")

# Project target
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp include/*.hpp)

add_executable(${PROJECT_NAME}
    ${SOURCES}
)

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    NativeLifters-Core
    args
	asmjit::asmjit
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME vtil
)

# Set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Installation
install(TARGETS ${PROJECT_NAME})