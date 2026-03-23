#pragma once

#include "common.h"
#include "mesh_loader.h"

#include <cstdint>
#include <vector>

/// Per-cluster geometry: local vertex buffer, indices, bounds, LOD sphere.
/// Cluster is the atomic render unit; triangles within share vertices.
struct Cluster {
  std::vector<Vec3> positions;
  std::vector<uint32_t> indices;
  AABB bounds;
  Sphere lodSphere;
  float lodError = 0.f;
  float edgeLength = 0.f;
};

/// Page = collection of clusters (up to 511, hardware limit). One page per LOD
/// group; multiple pages per mip level if cluster count exceeds limit.
struct ClusterPage {
  std::vector<Cluster> clusters;
  int mipLevel = 0;
  AABB bounds;
  Sphere lodSphere;
};

/// Build result: pages (split by mip + cluster cap), mip level list.
struct BuildResult {
  std::vector<ClusterPage> pages;
  std::vector<int> mipLevels;
};

BuildResult BuildClustersAndPages(const LoadedMesh &mesh,
                                  const std::vector<int> &mipValues,
                                  uint32_t trianglesPerCluster,
                                  uint32_t maxClustersPerMip);
