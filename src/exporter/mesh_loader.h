#pragma once

#include "common.h"

#include <string>
#include <vector>

struct LoadedMesh {
  std::vector<Vec3> positions;
  std::vector<Triangle> triangles;
};

LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent);
