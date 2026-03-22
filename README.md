# Nano

A Vulkan-based virtual geometry system inspired by UE5 Nanite.

Implements BVH-based LOD selection, cluster culling, hardware rasterization with VisBuffer, and per-cluster visualization.

LOD 0:
![demo](demo.png)

LOD 3:
![demo_1](demo_1.png)

## Build

```bash
# Compile shaders
cd shaders && chmod +x compile.sh && ./compile.sh && cd ..

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
  exporter/             - Nanite BVH & NaniteMesh exporter (still under development)
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
6. **BuildHZB** (Compute) - Mip0 from VisBuffer64 depth, then max-reduction mips (previous-frame occlusion pyramid for step 2)
7. **SwapChain** (Graphics) - Blit visualization texture to screen

`GlobalConstants.mMisc0`: **x** = manual LOD mip, **y** = HZB occlusion (0 first frame / warmup, 1 once prior frame depth exists), **z/w** = screen size for projection tests.

## TODO

1. Using Multithreading for BVH traversal and other tasks.
3. Implement LOD selection based on distance to camera (maybe also a roaming camera...).
4. Finish the exporter module to support exporting BVH and NaniteMesh data.
