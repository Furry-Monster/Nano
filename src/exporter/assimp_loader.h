#pragma once

#include "common.h"

#include <string>
#include <vector>

struct LoadedMesh {
  std::vector<Vec3> positions;   // global unique positions
  std::vector<Triangle> triangles; // indices into positions
};

LoadedMesh LoadMeshAssimp(const std::string& inputPath, float targetExtent);

