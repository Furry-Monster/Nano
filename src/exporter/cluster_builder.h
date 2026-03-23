#pragma once

#include "common.h"
#include "mesh_loader.h"

#include <cstdint>
#include <vector>

struct Cluster {
  std::vector<Vec3> positions;
  std::vector<uint32_t> indices;
  AABB bounds;
  Sphere lodSphere;
  float lodError = 0.f;
  float edgeLength = 0.f;
};

struct ClusterPage {
  std::vector<Cluster> clusters;
  int mipLevel = 0;
  AABB bounds;
  Sphere lodSphere;
};

struct BuildResult {
  std::vector<ClusterPage> pages;
  std::vector<int> mipLevels;
};

BuildResult BuildClustersAndPages(const LoadedMesh &mesh,
                                  const std::vector<int> &mipValues,
                                  uint32_t trianglesPerCluster,
                                  uint32_t maxClustersPerMip);
