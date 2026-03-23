#pragma once

#include "common.h"

#include <string>
#include <vector>

struct LoadedMesh {
  std::vector<Vec3> positions;
  std::vector<Triangle> triangles;
};

/// Load mesh from FBX, GLTF, or GLB. Uses Assimp with format-appropriate flags.
LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent);
