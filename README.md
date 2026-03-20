# Nano

A Vulkan-based virtual geometry system inspired by UE5 Nanite.

Implements BVH-based LOD selection, cluster culling, hardware rasterization with VisBuffer, and per-cluster visualization.

![demo](demo.png)

## Build

```bash
# Compile shaders
cd shaders && chmod +x build.sh && ./build.sh && cd ..

# Build
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

## Run

```bash
cd bin && ./Nano
```

Use **Up/Down** arrow keys to switch LOD mip levels.

## Dependencies

- CMake 3.20+
- C++17 (Clang recommended, MSVC not supported)
- Vulkan SDK 1.2+
- GLFW
- glslc (from Vulkan SDK)

## Project Structure

```
src/
  main.cpp              - GLFW window + main loop
  math/                 - Custom math library (float4, matrix4, quaternion)
  render/               - Vulkan RHI, render passes, materials, meshes
  scene/                - Scene management and node hierarchy
shaders/                - GLSL compute and graphics shaders
res/                    - Runtime assets (BVH, Nanite mesh data)
libs/                   - Third-party libraries (GLFW, GLM, ImGui, spdlog, stb)
```

## Rendering Pipeline

1. **Init** (Compute) - Clear VisBuffer64, initialize work arguments
2. **NodeAndClusterCull** (Compute x4) - BVH traversal, collect visible clusters by LOD
3. **ClusterCull** (Compute) - Copy visible clusters to output buffer
4. **HWRasterize** (Graphics, Indirect) - Rasterize clusters, write depth+clusterID to VisBuffer64 via atomicMin
5. **Visualize** (Compute) - Convert VisBuffer64 cluster IDs to colors via MurmurHash
6. **SwapChain** (Graphics) - Blit visualization texture to screen
