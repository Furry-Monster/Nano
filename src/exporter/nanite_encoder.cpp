#include "nanite_encoder.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

static std::vector<Triangle> Decimate(const std::vector<Triangle>& tris,
                                       uint64_t step) {
  if (step == 0) return tris;
  std::vector<Triangle> out;
  out.reserve(tris.size() / std::max<uint64_t>(1, step) + 1);
  for (size_t i = 0; i < tris.size(); ++i) {
    if ((i % step) == 0) out.push_back(tris[i]);
  }
  return out;
}

std::vector<ExportPage> BuildPages(
    const std::vector<Vec3>& positions, const std::vector<Triangle>& triangles,
    const std::vector<int>& mipValues, uint32_t trianglesPerCluster,
    uint32_t indexCount, uint32_t maxClustersPerMip) {
  std::vector<ExportPage> pages;
  pages.resize(mipValues.size());

  // Reusable global-to-local mark table.
  std::vector<int32_t> g2l(positions.size(), -1);
  std::vector<uint32_t> usedGlobal;
  usedGlobal.reserve(1024);

  for (size_t mipIdx = 0; mipIdx < mipValues.size(); ++mipIdx) {
    const uint64_t lodIndex = mipIdx;
    const uint64_t baseStep = 1ull << std::min<uint64_t>(lodIndex, 30);
    auto trisMip = Decimate(triangles, baseStep);

    if (trisMip.empty()) {
      trisMip.assign(triangles.begin(),
                      triangles.begin() +
                          std::min<size_t>(triangles.size(), trianglesPerCluster));
    }

    // Clamp cluster count (keep within leaf/page limits in shader).
    const uint32_t maxTrisForMip = maxClustersPerMip * trianglesPerCluster;
    if (trisMip.size() > maxTrisForMip) {
      const uint64_t extraFactor =
          static_cast<uint64_t>((trisMip.size() + maxTrisForMip - 1) /
                                 maxTrisForMip);
      trisMip = Decimate(trisMip, extraFactor);
    }
    if (trisMip.empty()) trisMip = {triangles[0]};

    const uint32_t clusterCount =
        static_cast<uint32_t>((trisMip.size() + trianglesPerCluster - 1) /
                               trianglesPerCluster);
    if (clusterCount == 0) {
      throw std::runtime_error("Internal error: clusterCount==0");
    }
    (void)clusterCount;

    if (clusterCount > maxClustersPerMip) {
      trisMip.resize(static_cast<size_t>(maxClustersPerMip) * trianglesPerCluster);
    }

    std::vector<ExportCluster> clusters;
    clusters.reserve(static_cast<size_t>(
        std::min<uint32_t>(clusterCount, maxClustersPerMip)));

    const uint32_t localTriCount = trianglesPerCluster;
    for (uint32_t ci = 0; ci < maxClustersPerMip; ++ci) {
      size_t start = static_cast<size_t>(ci) * localTriCount;
      if (start >= trisMip.size()) break;
      size_t end = std::min(start + localTriCount, trisMip.size());

      ExportCluster cl;
      cl.positions.clear();
      cl.indexRemap.assign(indexCount, 0);
      usedGlobal.clear();

      // Build local vertex set from real triangles.
      for (size_t ti = start; ti < end; ++ti) {
        const Triangle& t = trisMip[ti];
        const uint32_t ids[3] = {t.a, t.b, t.c};
        for (int k = 0; k < 3; ++k) {
          const uint32_t gid = ids[k];
          if (g2l[gid] == -1) {
            g2l[gid] = static_cast<int32_t>(cl.positions.size());
            usedGlobal.push_back(gid);
            cl.positions.push_back(positions[gid]);
          }
        }
      }

      if (cl.positions.empty()) {
        cl.positions.push_back({0, 0, 0});
      }

      const uint32_t padLocalIndex = 0;

      // Fill indexRemap as triangle-list vertex stream.
      size_t outVertex = 0; // in [0..indexCount)
      for (size_t ti = start; ti < end; ++ti) {
        const Triangle& t = trisMip[ti];
        const uint32_t ids[3] = {t.a, t.b, t.c};
        for (int k = 0; k < 3; ++k) {
          if (outVertex >= indexCount) break;
          const uint32_t gid = ids[k];
          cl.indexRemap[outVertex++] = static_cast<uint32_t>(g2l[gid]);
        }
        if (outVertex >= indexCount) break;
      }

      // Explicitly ensure padding uses a valid vertex index.
      for (; outVertex < indexCount; ++outVertex) {
        cl.indexRemap[outVertex] = padLocalIndex;
      }

      for (uint32_t gid : usedGlobal) g2l[gid] = -1;

      clusters.push_back(std::move(cl));
    }

    pages[mipIdx].clusters = std::move(clusters);
  }

  return pages;
}

void EncodeNaniteMesh(const std::vector<ExportPage>& pages, uint32_t indexCount,
                       const std::string& outPath) {
  // Shader: HWRasterizeVS.glsl
  // GetClusterInfo() expects:
  // - pageCount
  // - per page: clusterCountOnPage + clusterBaseOffsetInBytesLocal[...]
  // - per cluster: indexDataOffsetBytesLocal, indexCount, lodBounds(4 floats as uint bits),
  //   lodErrorAndEdgeLength packed, positions(numVerts*3 floats bits), indexRemap(indexCount u32).
  constexpr uint32_t kIndexDataOffsetBaseBytes = 28; // fixed part before positions

  std::vector<uint32_t> naniteU32;
  naniteU32.reserve(1024 * 1024);

  naniteU32.push_back(static_cast<uint32_t>(pages.size())); // pageCount
  naniteU32.resize(1 + pages.size(), 0); // pageBaseOffsetInBytes placeholders

  for (uint32_t pi = 0; pi < pages.size(); ++pi) {
    const auto& page = pages[pi];
    const uint32_t clusterCountOnPage =
        static_cast<uint32_t>(page.clusters.size());
    if (clusterCountOnPage == 0) continue;

    uint32_t pageBaseOffsetBytes = static_cast<uint32_t>(naniteU32.size() * 4ull);
    naniteU32[1 + pi] = pageBaseOffsetBytes;

    naniteU32.push_back(clusterCountOnPage);

    const size_t clusterOffsetsStart = naniteU32.size();
    naniteU32.resize(naniteU32.size() + clusterCountOnPage, 0);

    uint32_t clusterDataOffsetBytes = 0;
    for (uint32_t ci = 0; ci < clusterCountOnPage; ++ci) {
      naniteU32[clusterOffsetsStart + ci] = clusterDataOffsetBytes;

      const ExportCluster& cl = page.clusters[ci];
      const uint32_t numVerts = static_cast<uint32_t>(cl.positions.size());

      const uint32_t indexDataOffsetBytesLocal =
          kIndexDataOffsetBaseBytes + numVerts * (sizeof(float) * 3u);

      // cluster block header (7 uints)
      naniteU32.push_back(indexDataOffsetBytesLocal);
      naniteU32.push_back(indexCount);
      naniteU32.push_back(0); // lodBounds x
      naniteU32.push_back(0); // lodBounds y
      naniteU32.push_back(0); // lodBounds z
      naniteU32.push_back(0); // lodBounds w
      naniteU32.push_back(0); // lodErrorAndEdgeLength

      // positions: uint bits of float32, 3 per vertex
      for (const auto& p : cl.positions) {
        naniteU32.push_back(FloatToU32(p.x));
        naniteU32.push_back(FloatToU32(p.y));
        naniteU32.push_back(FloatToU32(p.z));
      }

      if (cl.indexRemap.size() != indexCount) {
        throw std::runtime_error("Internal error: indexRemap size mismatch.");
      }
      for (uint32_t k = 0; k < indexCount; ++k) {
        naniteU32.push_back(cl.indexRemap[k]);
      }

      const uint32_t clusterBlockUintCount =
          7u + numVerts * 3u + indexCount;
      clusterDataOffsetBytes += clusterBlockUintCount * 4u;
    }
  }

  WriteU32File(outPath, naniteU32);
}

void EncodeBVH(const std::vector<ExportPage>& pages,
               const std::vector<int>& mipValues, const std::string& outPath) {
  // Shader: NodeAndClusterCull.glsl
  constexpr uint32_t kNodes = 5;
  constexpr uint32_t kNodeUintCount = 52; // matches HIERARCHY_NODE_SLICE_SIZE decoding
  constexpr uint32_t kInternalMisc2 = 0xFFFFFFFFu;
  constexpr uint32_t kLeafStartPageIndex = 0;

  std::vector<uint32_t> bvhU32(kNodes * kNodeUintCount, 0);
  auto SetSlice = [&](uint32_t nodeIndex, uint32_t childIndex,
                       uint32_t raw2w_childStart, uint32_t misc2) {
    const uint32_t base = nodeIndex * kNodeUintCount;
    const uint32_t raw2Start = base + 32u + 4u * childIndex;
    const uint32_t raw3Index = base + 48u + childIndex;

    bvhU32[raw2Start + 0] = 0; // extent x
    bvhU32[raw2Start + 1] = 0; // extent y
    bvhU32[raw2Start + 2] = 0; // extent z
    bvhU32[raw2Start + 3] = raw2w_childStart; // ChildStartReference
    bvhU32[raw3Index] = misc2;                    // Packed misc2
  };

  // node 0 root: 4 internal slices -> node 1..4
  for (uint32_t c = 0; c < 4; ++c) {
    SetSlice(0, c, /*childStart*/ (1u + c), /*misc2*/ kInternalMisc2);
  }

  // nodes 1..4: leaf slices for mipValues
  for (uint32_t nodeIndex = 1; nodeIndex <= 4; ++nodeIndex) {
    for (uint32_t childIndex = 0; childIndex < 4; ++childIndex) {
      const uint32_t leafIdx = (nodeIndex - 1) * 4u + childIndex;
      if (leafIdx >= mipValues.size() || leafIdx >= pages.size()) {
        SetSlice(nodeIndex, childIndex, 0u, 0u); // disabled
        continue;
      }

      const uint32_t mipValue = static_cast<uint32_t>(mipValues[leafIdx]);
      const uint32_t pageIndex = leafIdx;
      const uint32_t clusterCountOnPage =
          static_cast<uint32_t>(pages[leafIdx].clusters.size());
      if (clusterCountOnPage == 0) {
        SetSlice(nodeIndex, childIndex, 0u, 0u);
        continue;
      }
      if (clusterCountOnPage > 511u) {
        throw std::runtime_error(
            "clusterCountOnPage exceeds 9-bit NumChildren (<=511).");
      }

      const uint32_t childStartReference = (pageIndex << 8) | 0u;
      const uint32_t misc2 =
          PackMisc2(clusterCountOnPage, mipValue, kLeafStartPageIndex);
      SetSlice(nodeIndex, childIndex, childStartReference, misc2);
    }
  }

  WriteU32File(outPath, bvhU32);
}

