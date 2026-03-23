#pragma once

#include "common.h"

#include <string>
#include <vector>

struct LoadedMesh {
  std::vector<Vec3> positions;
  std::vector<Triangle> triangles;
};

/// Load mesh from FBX, GLTF, or GLB via Assimp. Centers and scales to
/// targetExtent. Removes degenerate triangles.
LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent);
