#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Compiling shaders..."

glslc -fshader-stage=compute -o Init.sb Init.glsl
glslc -fshader-stage=compute -o NodeAndClusterCull.sb NodeAndClusterCull.glsl
glslc -fshader-stage=compute -o ClusterCull.sb ClusterCull.glsl
glslc -fshader-stage=compute -o Visualize.sb Visualize.glsl
glslc -fshader-stage=vertex -o HWRasterizeVS.sb HWRasterizeVS.glsl
glslc -fshader-stage=fragment -o HWRasterizeFS.sb HWRasterizeFS.glsl
glslc -fshader-stage=vertex -o swapchainVS.sb swapchainVS.glsl
glslc -fshader-stage=fragment -o swapchainFS.sb swapchainFS.glsl

echo "All shaders compiled successfully."
