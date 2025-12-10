#!/bin/bash

SHADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SHADER_DIR}"

if ! command -v glslc &> /dev/null; then
    echo "Failure: unable to find glslc. Please install Vulkan SDK or glslang-tools"
    exit 1
fi

echo "Compile Compute Shaders..."
glslc -fshader-stage=compute -o "${OUTPUT_DIR}/Init.sb" "${SHADER_DIR}/Init.glsl"
glslc -fshader-stage=compute -o "${OUTPUT_DIR}/NodeAndClusterCull.sb" "${SHADER_DIR}/NodeAndClusterCull.glsl"
glslc -fshader-stage=compute -o "${OUTPUT_DIR}/ClusterCull.sb" "${SHADER_DIR}/ClusterCull.glsl"
glslc -fshader-stage=compute -o "${OUTPUT_DIR}/Visualize.sb" "${SHADER_DIR}/Visualize.glsl"

echo "Compile Shaders..."
glslc -fshader-stage=vertex -o "${OUTPUT_DIR}/HWRasterizeVS.sb" "${SHADER_DIR}/HWRasterizeVS.glsl"
glslc -fshader-stage=fragment -o "${OUTPUT_DIR}/HWRasterizeFS.sb" "${SHADER_DIR}/HWRasterizeFS.glsl"
glslc -fshader-stage=vertex -o "${OUTPUT_DIR}/swapchainVS.sb" "${SHADER_DIR}/swapchainVS.glsl"
glslc -fshader-stage=fragment -o "${OUTPUT_DIR}/swapchainFS.sb" "${SHADER_DIR}/swapchainFS.glsl"

echo "Finish Shader Compilation！"
