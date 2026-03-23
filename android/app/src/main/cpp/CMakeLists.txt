cmake_minimum_required(VERSION 3.22.1)
project("nano_android")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NANO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

set(NANO_SOURCES
    ${NANO_ROOT}/src/input/input.cpp
    ${NANO_ROOT}/src/math/float4.cpp
    ${NANO_ROOT}/src/math/matrix4.cpp
    ${NANO_ROOT}/src/math/quaternion.cpp
    ${NANO_ROOT}/src/render/vulkan_rhi.cpp
    ${NANO_ROOT}/src/render/render_pass.cpp
    ${NANO_ROOT}/src/render/material.cpp
    ${NANO_ROOT}/src/render/static_mesh.cpp
    ${NANO_ROOT}/src/scene/scene.cpp
    ${NANO_ROOT}/src/scene/scene_node.cpp
    ${NANO_ROOT}/src/exporter/export_android.cpp
    ${NANO_ROOT}/src/exporter/mesh_loader.cpp
    ${NANO_ROOT}/src/exporter/cluster_builder.cpp
    ${NANO_ROOT}/src/exporter/nanite_encode.cpp
)

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    ${NANO_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${NANO_ROOT}/src
    ${NANO_ROOT}/src/exporter
)

# Assimp (FBX/GLTF import for exporter)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "..." FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "..." FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "..." FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "..." FORCE)
add_subdirectory(${NANO_ROOT}/libs/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp)

# MeshOptimizer (for exporter)
set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "..." FORCE)
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "..." FORCE)
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "..." FORCE)
add_subdirectory(${NANO_ROOT}/libs/meshoptimizer ${CMAKE_CURRENT_BINARY_DIR}/meshopt)

# spdlog (header-only mode to avoid building the static lib)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${NANO_ROOT}/libs/spdlog ${CMAKE_CURRENT_BINARY_DIR}/spdlog)

# glm (header-only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${NANO_ROOT}/libs/glm)

# stb (header-only)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${NANO_ROOT}/libs/stb)

find_library(vulkan-lib vulkan)

target_link_libraries(${CMAKE_PROJECT_NAME}
    android
    log
    ${vulkan-lib}
    spdlog
    glm
    stb
    assimp::assimp
    meshoptimizer
)
