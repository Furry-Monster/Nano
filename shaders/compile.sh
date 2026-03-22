#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Compiling shaders..."

glslc -fshader-stage=compute -o Init.sb Init.glsl
glslc -fshader-stage=compute -o NodeAndClusterCull.sb NodeAndClusterCull.glsl
glslc -fshader-stage=compute -o ClusterCull.sb ClusterCull.glsl
glslc -fshader-stage=compute -o Visualize.sb Visualize.glsl
glslc -fshader-stage=compute -o BuildHZBMip0.sb BuildHZBMip0.glsl
glslc -fshader-stage=compute -o BuildHZBDownsample.sb BuildHZBDownsample.glsl
glslc -fshader-stage=vertex -o HWRasterizeVS.sb HWRasterizeVS.glsl
glslc -fshader-stage=fragment -o HWRasterizeFS.sb HWRasterizeFS.glsl
glslc -fshader-stage=vertex -o SwapchainVS.sb SwapchainVS.glsl
glslc -fshader-stage=fragment -o SwapchainFS.sb SwapchainFS.glsl

echo "All shaders compiled successfully."
