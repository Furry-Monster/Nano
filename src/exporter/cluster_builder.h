#pragma once

#include "common.h"
#include "mesh_loader.h"

#include <cstdint>
#include <vector>

struct Cluster {
  std::vector<Vec3> positions;
  std::vector<uint32_t> indices;  // triangle list into positions
  Vec3 boundsMin;
  Vec3 boundsMax;
  float lodError = 0.f;
  float edgeLength = 0.f;
};

struct ClusterPage {
  std::vector<Cluster> clusters;
  int mipLevel = 0;
};

struct BuildResult {
  std::vector<ClusterPage> pages;
  std::vector<int> mipLevels;
};

/// Build clusters and pages from mesh. LOD via decimation; clusters grouped by mip.
BuildResult BuildClustersAndPages(const LoadedMesh &mesh,
                                  const std::vector<int> &mipValues,
                                  uint32_t trianglesPerCluster,
                                  uint32_t maxClustersPerMip);
