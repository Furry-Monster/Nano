#pragma once

#include "common.h"

#include <string>
#include <vector>

/// Loaded mesh: positions and triangles. Assimp merges all submeshes,
/// centers and scales to targetExtent, removes degenerate triangles.
struct LoadedMesh {
  std::vector<Vec3> positions;
  std::vector<Triangle> triangles;
};

LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent);
