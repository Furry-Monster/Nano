#include "nanite_encode.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace {

// Match NodeAndClusterCull.glsl HIERARCHY_NODE_SLICE_SIZE
constexpr uint32_t kHierarchyNodeSliceSize = 52;  // uints per node
constexpr uint32_t kMaxBvhFanout = 4;

inline uint32_t FloatBits(float v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(float));
  return u;
}

// Simple half encode for small floats
uint16_t FloatToHalfSimple(float v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(uint32_t));
  if (u == 0) return 0;
  const uint32_t sign = (u >> 31) << 15;
  const uint32_t exp = ((u >> 23) & 0xFF);
  if (exp == 0xFF) return static_cast<uint16_t>(sign | 0x7C00u);
  const int32_t iexp = static_cast<int32_t>(exp) - 127;
  if (iexp < -24) return static_cast<uint16_t>(sign);
  if (iexp > 15) return static_cast<uint16_t>(sign | 0x7C00u);
  const uint32_t mantissa = (u >> 13) & 0x3FFu;
  const uint32_t hexp = static_cast<uint32_t>(iexp + 15) << 10;
  return static_cast<uint16_t>(sign | hexp | mantissa);
}

void SetChildSliceWithHalf(std::vector<uint32_t> &bvh, uint32_t nodeIndex,
                           uint32_t childIndex, float lodCenterX,
                           float lodCenterY, float lodCenterZ, float lodRadius,
                           float boxCenterX, float boxCenterY, float boxCenterZ,
                           float boxExtentX, float boxExtentY, float boxExtentZ,
                           float minLodError, float maxParentLodError,
                           uint32_t childStartRef, uint32_t misc2) {
  const uint32_t base = nodeIndex * kHierarchyNodeSliceSize;
  const uint32_t r0Base = base + childIndex * 4;
  bvh[r0Base + 0] = FloatBits(lodCenterX);
  bvh[r0Base + 1] = FloatBits(lodCenterY);
  bvh[r0Base + 2] = FloatBits(lodCenterZ);
  bvh[r0Base + 3] = FloatBits(lodRadius);

  const uint32_t r1Base = base + 16 + childIndex * 4;
  bvh[r1Base + 0] = FloatBits(boxCenterX);
  bvh[r1Base + 1] = FloatBits(boxCenterY);
  bvh[r1Base + 2] = FloatBits(boxCenterZ);
  bvh[r1Base + 3] =
      static_cast<uint32_t>(FloatToHalfSimple(minLodError)) |
      (static_cast<uint32_t>(FloatToHalfSimple(maxParentLodError)) << 16);

  const uint32_t r2Base = base + 32 + childIndex * 4;
  bvh[r2Base + 0] = FloatBits(boxExtentX);
  bvh[r2Base + 1] = FloatBits(boxExtentY);
  bvh[r2Base + 2] = FloatBits(boxExtentZ);
  bvh[r2Base + 3] = childStartRef;

  bvh[base + 48 + childIndex] = misc2;
}

void ComputePageBounds(const ClusterPage &page, Vec3 &outCenter, Vec3 &outExtent,
                       float &outLodRadius) {
  Vec3 bmin{1e30f, 1e30f, 1e30f};
  Vec3 bmax{-1e30f, -1e30f, -1e30f};
  for (const auto &cl : page.clusters) {
    bmin.x = std::min(bmin.x, cl.boundsMin.x);
    bmin.y = std::min(bmin.y, cl.boundsMin.y);
    bmin.z = std::min(bmin.z, cl.boundsMin.z);
    bmax.x = std::max(bmax.x, cl.boundsMax.x);
    bmax.y = std::max(bmax.y, cl.boundsMax.y);
    bmax.z = std::max(bmax.z, cl.boundsMax.z);
  }
  outCenter.x = (bmin.x + bmax.x) * 0.5f;
  outCenter.y = (bmin.y + bmax.y) * 0.5f;
  outCenter.z = (bmin.z + bmax.z) * 0.5f;
  outExtent.x = (bmax.x - bmin.x) * 0.5f;
  outExtent.y = (bmax.y - bmin.y) * 0.5f;
  outExtent.z = (bmax.z - bmin.z) * 0.5f;
  const float dx = bmax.x - bmin.x, dy = bmax.y - bmin.y, dz = bmax.z - bmin.z;
  outLodRadius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

void EncodeBVH(const BuildResult &build, const std::string &outPath) {
  const size_t numPages = build.pages.size();
  if (numPages == 0) {
    throw std::runtime_error("No pages to encode");
  }

  const uint32_t numLeaves = static_cast<uint32_t>(numPages);
  // Two-level BVH: root (node 0) has 4 children (nodes 1-4), each with up to 4 leaf children.
  // Supports up to 16 pages. If more, we only encode first 16.
  const uint32_t maxPages = std::min(numLeaves, 16u);
  const uint32_t numInternalNodes = 5;  // root + 4 children
  std::vector<uint32_t> bvh(numInternalNodes * kHierarchyNodeSliceSize, 0);

  for (uint32_t parent = 0; parent < 4; ++parent) {
    const uint32_t nodeIndex = 1 + parent;
    const uint32_t pageStart = parent * 4;
    const uint32_t pageEnd = std::min(pageStart + 4, maxPages);

    if (pageStart >= maxPages) {
      for (uint32_t c = 0; c < kMaxBvhFanout; ++c) {
        SetChildSliceWithHalf(bvh, nodeIndex, c, 0, 0, 0, 1.f, 0, 0, 0, 0, 0, 0,
                              0.f, 0.f, 0xFFFFFFFFu, 0u);
      }
      continue;
    }

    Vec3 groupCenter{0, 0, 0}, groupExtent{0, 0, 0};
    float groupRadius = 0.f;
    int numGroupPages = 0;

    for (uint32_t c = 0; c < kMaxBvhFanout; ++c) {
      const uint32_t pageIdx = pageStart + c;
      if (pageIdx >= pageEnd) {
        SetChildSliceWithHalf(bvh, nodeIndex, c, 0, 0, 0, 1.f, 0, 0, 0, 0, 0, 0,
                              0.f, 0.f, 0xFFFFFFFFu, 0u);
        continue;
      }

      const auto &page = build.pages[pageIdx];
      Vec3 center, extent;
      float lodRadius;
      ComputePageBounds(page, center, extent, lodRadius);
      groupCenter.x += center.x;
      groupCenter.y += center.y;
      groupCenter.z += center.z;
      groupExtent.x = std::max(groupExtent.x, extent.x);
      groupExtent.y = std::max(groupExtent.y, extent.y);
      groupExtent.z = std::max(groupExtent.z, extent.z);
      groupRadius = std::max(groupRadius, lodRadius);
      numGroupPages++;

      const uint32_t clusterCount =
          static_cast<uint32_t>(page.clusters.size());
      if (clusterCount > 511u) {
        throw std::runtime_error("clusterCount > 511");
      }
      const uint32_t childStartRef = (pageIdx << 8) | 0u;
      const uint32_t misc2 = PackMisc2Leaf(clusterCount, page.mipLevel, 0);
      SetChildSliceWithHalf(bvh, nodeIndex, c, center.x, center.y, center.z,
                            lodRadius, center.x, center.y, center.z,
                            extent.x, extent.y, extent.z, 0.f, 1e10f,
                            childStartRef, misc2);
    }

    if (numGroupPages > 0) {
      groupCenter.x /= numGroupPages;
      groupCenter.y /= numGroupPages;
      groupCenter.z /= numGroupPages;
    }

    // Root's child points to node 1-4
    SetChildSliceWithHalf(bvh, 0, parent, groupCenter.x, groupCenter.y,
                          groupCenter.z, groupRadius, groupCenter.x,
                          groupCenter.y, groupCenter.z, groupExtent.x,
                          groupExtent.y, groupExtent.z, 0.f, 1e10f, nodeIndex,
                          0xFFFFFFFFu);
  }

  WriteU32File(outPath, bvh);
}

void EncodeNaniteMesh(const BuildResult &build, uint32_t indexCountPerCluster,
                      const std::string &outPath) {
  std::vector<uint32_t> out;
  out.reserve(4 * 1024 * 1024);

  const uint32_t pageCount = static_cast<uint32_t>(build.pages.size());
  out.push_back(pageCount);
  out.resize(1 + pageCount, 0);

  uint32_t runningOffsetBytes = (1 + pageCount) * 4u;

  for (uint32_t pi = 0; pi < pageCount; ++pi) {
    const auto &page = build.pages[pi];
    out[1 + pi] = runningOffsetBytes;

    const uint32_t clusterCount =
        static_cast<uint32_t>(page.clusters.size());
    if (clusterCount == 0) continue;

    out.push_back(clusterCount);
    const size_t clusterOffsetsStart = out.size();
    out.resize(out.size() + clusterCount, 0);

    uint32_t clusterDataOffsetBytes = 0;

    for (uint32_t ci = 0; ci < clusterCount; ++ci) {
      const auto &cl = page.clusters[ci];
      out[clusterOffsetsStart + ci] = clusterDataOffsetBytes;

      const uint32_t numVerts = static_cast<uint32_t>(cl.positions.size());
      const uint32_t indexDataOffsetLocal =
          7 * 4u + numVerts * 3u * 4u;

      out.push_back(indexDataOffsetLocal);
      out.push_back(static_cast<uint32_t>(cl.indices.size()));
      const float cx = (cl.boundsMin.x + cl.boundsMax.x) * 0.5f;
      const float cy = (cl.boundsMin.y + cl.boundsMax.y) * 0.5f;
      const float cz = (cl.boundsMin.z + cl.boundsMax.z) * 0.5f;
      const float r = 0.5f * std::sqrt(
          (cl.boundsMax.x - cl.boundsMin.x) * (cl.boundsMax.x - cl.boundsMin.x) +
          (cl.boundsMax.y - cl.boundsMin.y) * (cl.boundsMax.y - cl.boundsMin.y) +
          (cl.boundsMax.z - cl.boundsMin.z) * (cl.boundsMax.z - cl.boundsMin.z));
      out.push_back(FloatBits(cx));
      out.push_back(FloatBits(cy));
      out.push_back(FloatBits(cz));
      out.push_back(FloatBits(r));
      out.push_back(PackLodErrorEdgeLength(cl.lodError, cl.edgeLength));

      for (const auto &p : cl.positions) {
        out.push_back(FloatBits(p.x));
        out.push_back(FloatBits(p.y));
        out.push_back(FloatBits(p.z));
      }

      std::vector<uint32_t> indexRemap(indexCountPerCluster, 0);
      const uint32_t copyCount =
          std::min(static_cast<uint32_t>(cl.indices.size()), indexCountPerCluster);
      for (uint32_t k = 0; k < copyCount; ++k) {
        indexRemap[k] = cl.indices[k];
      }
      for (uint32_t k = copyCount; k < indexCountPerCluster; ++k) {
        indexRemap[k] = 0;
      }
      for (uint32_t k = 0; k < indexCountPerCluster; ++k) {
        out.push_back(indexRemap[k]);
      }

      clusterDataOffsetBytes += (7 + numVerts * 3 + indexCountPerCluster) * 4u;
    }

    runningOffsetBytes += 4u + clusterCount * 4u + clusterDataOffsetBytes;
  }

  WriteU32File(outPath, out);
}
