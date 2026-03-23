#include "cluster_builder.h"

#include <meshoptimizer.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kEpsilon = 1e-9f;
constexpr uint32_t kMaxTotalPages = 16;

float EdgeLen(const Vec3 &a, const Vec3 &b) { return (b - a).length(); }

// ---- Morton code for 3D spatial sorting (10 bits per axis) ----------------

uint32_t ExpandBits10(uint32_t v) {
  v &= 0x3FFu;
  v = (v | (v << 16)) & 0x030000FFu;
  v = (v | (v << 8)) & 0x0300F00Fu;
  v = (v | (v << 4)) & 0x030C30C3u;
  v = (v | (v << 2)) & 0x09249249u;
  return v;
}

uint32_t Morton3D(uint32_t x, uint32_t y, uint32_t z) {
  return ExpandBits10(x) | (ExpandBits10(y) << 1) | (ExpandBits10(z) << 2);
}

void SortTrianglesSpatially(std::vector<Triangle> &tris,
                            const std::vector<Vec3> &positions) {
  if (tris.size() <= 1)
    return;

  AABB centroidBox;
  for (const auto &t : tris) {
    Vec3 c = (positions[t.a] + positions[t.b] + positions[t.c]) / 3.f;
    centroidBox.expand(c);
  }

  Vec3 range = centroidBox.hi - centroidBox.lo;
  float rx = range.x > kEpsilon ? range.x : 1.f;
  float ry = range.y > kEpsilon ? range.y : 1.f;
  float rz = range.z > kEpsilon ? range.z : 1.f;

  std::vector<uint32_t> mortonCodes(tris.size());
  for (size_t i = 0; i < tris.size(); ++i) {
    const Triangle &t = tris[i];
    Vec3 c = (positions[t.a] + positions[t.b] + positions[t.c]) / 3.f;
    auto mx = static_cast<uint32_t>(
        std::clamp((c.x - centroidBox.lo.x) / rx, 0.f, 1.f) * 1023.f);
    auto my = static_cast<uint32_t>(
        std::clamp((c.y - centroidBox.lo.y) / ry, 0.f, 1.f) * 1023.f);
    auto mz = static_cast<uint32_t>(
        std::clamp((c.z - centroidBox.lo.z) / rz, 0.f, 1.f) * 1023.f);
    mortonCodes[i] = Morton3D(mx, my, mz);
  }

  std::vector<size_t> order(tris.size());
  std::iota(order.begin(), order.end(), size_t(0));
  std::sort(order.begin(), order.end(),
            [&](size_t a, size_t b) { return mortonCodes[a] < mortonCodes[b]; });

  std::vector<Triangle> sorted(tris.size());
  for (size_t i = 0; i < tris.size(); ++i)
    sorted[i] = tris[order[i]];
  tris = std::move(sorted);
}

// ---- Mesh simplification via meshoptimizer --------------------------------

struct SimplifiedMesh {
  std::vector<Vec3> positions;
  std::vector<Triangle> triangles;
};

SimplifiedMesh SimplifyMesh(const std::vector<Vec3> &srcPositions,
                            const std::vector<Triangle> &srcTriangles,
                            size_t targetTriCount) {
  if (srcTriangles.size() <= targetTriCount) {
    return {srcPositions, srcTriangles};
  }

  std::vector<unsigned int> indices;
  indices.reserve(srcTriangles.size() * 3);
  for (const auto &t : srcTriangles) {
    indices.push_back(t.a);
    indices.push_back(t.b);
    indices.push_back(t.c);
  }

  size_t targetIndexCount = targetTriCount * 3;
  float targetError = 1e-1f;
  unsigned int options = meshopt_SimplifyLockBorder;

  std::vector<unsigned int> simplified(indices.size());
  size_t simplifiedIndexCount = meshopt_simplify(
      simplified.data(), indices.data(), indices.size(),
      reinterpret_cast<const float *>(srcPositions.data()), srcPositions.size(),
      sizeof(Vec3), targetIndexCount, targetError, options, nullptr);

  simplified.resize(simplifiedIndexCount);

  // Compact: remap to only the vertices actually used
  std::vector<unsigned int> remap(srcPositions.size(), UINT32_MAX);
  uint32_t newVertCount = 0;
  for (unsigned int idx : simplified) {
    if (remap[idx] == UINT32_MAX)
      remap[idx] = newVertCount++;
  }

  SimplifiedMesh result;
  result.positions.resize(newVertCount);
  for (size_t i = 0; i < srcPositions.size(); ++i) {
    if (remap[i] != UINT32_MAX)
      result.positions[remap[i]] = srcPositions[i];
  }

  result.triangles.reserve(simplifiedIndexCount / 3);
  for (size_t i = 0; i + 2 < simplifiedIndexCount; i += 3) {
    result.triangles.push_back(
        {remap[simplified[i]], remap[simplified[i + 1]],
         remap[simplified[i + 2]]});
  }

  return result;
}

// ---- Cluster construction -------------------------------------------------

std::vector<Cluster> MakeClusters(const std::vector<Triangle> &tris,
                                  const std::vector<Vec3> &positions,
                                  uint32_t trianglesPerCluster,
                                  float lodErrorEstimate) {
  std::vector<Cluster> clusters;

  std::vector<int32_t> globalToLocal(positions.size(), -1);
  std::vector<uint32_t> usedVerts;
  usedVerts.reserve(trianglesPerCluster * 3);

  const size_t numClusters =
      (tris.size() + trianglesPerCluster - 1) / trianglesPerCluster;

  for (size_t ci = 0; ci < numClusters; ++ci) {
    const size_t triStart = ci * trianglesPerCluster;
    const size_t triEnd =
        std::min(triStart + static_cast<size_t>(trianglesPerCluster),
                 tris.size());

    Cluster cl;
    usedVerts.clear();

    AABB box;
    float maxEdge = 0.f;

    for (size_t ti = triStart; ti < triEnd; ++ti) {
      const Triangle &t = tris[ti];
      const uint32_t ids[3] = {t.a, t.b, t.c};
      uint32_t localIds[3];

      for (int k = 0; k < 3; ++k) {
        const uint32_t gid = ids[k];
        if (globalToLocal[gid] < 0) {
          globalToLocal[gid] = static_cast<int32_t>(cl.positions.size());
          usedVerts.push_back(gid);
          const Vec3 &p = positions[gid];
          cl.positions.push_back(p);
          box.expand(p);
        }
        localIds[k] = static_cast<uint32_t>(globalToLocal[gid]);
      }

      cl.indices.push_back(localIds[0]);
      cl.indices.push_back(localIds[1]);
      cl.indices.push_back(localIds[2]);

      const Vec3 &va = cl.positions[localIds[0]];
      const Vec3 &vb = cl.positions[localIds[1]];
      const Vec3 &vc = cl.positions[localIds[2]];
      maxEdge = std::max(
          {maxEdge, EdgeLen(va, vb), EdgeLen(vb, vc), EdgeLen(vc, va)});
    }

    for (uint32_t gid : usedVerts)
      globalToLocal[gid] = -1;

    cl.bounds = box;
    cl.lodSphere =
        ComputeBoundingSphere(cl.positions.data(), cl.positions.size());
    cl.edgeLength = maxEdge;
    cl.lodError = lodErrorEstimate;

    clusters.push_back(std::move(cl));
  }

  return clusters;
}

} // namespace

BuildResult BuildClustersAndPages(const LoadedMesh &mesh,
                                  const std::vector<int> &mipValues,
                                  uint32_t trianglesPerCluster,
                                  uint32_t maxClustersPerMip) {
  if (mesh.triangles.empty() || mesh.positions.empty())
    throw std::runtime_error("Empty mesh");
  if (trianglesPerCluster == 0)
    throw std::runtime_error("trianglesPerCluster must be > 0");

  const float meshDiagonal = [&] {
    AABB box;
    for (const auto &p : mesh.positions)
      box.expand(p);
    return box.diagonal();
  }();

  struct RawMip {
    int mipLevel;
    std::vector<Cluster> clusters;
  };
  std::vector<RawMip> rawMips;

  for (size_t mipIdx = 0; mipIdx < mipValues.size(); ++mipIdx) {
    const int mipLevel = mipValues[mipIdx];

    const std::vector<Vec3> *posPtr = &mesh.positions;
    const std::vector<Triangle> *triPtr = &mesh.triangles;
    SimplifiedMesh simplified;

    if (mipLevel > 0) {
      float reductionFactor = std::pow(0.5f, static_cast<float>(mipLevel));
      size_t targetTriCount = std::max(
          size_t(1),
          static_cast<size_t>(mesh.triangles.size() * reductionFactor));

      simplified =
          SimplifyMesh(mesh.positions, mesh.triangles, targetTriCount);
      posPtr = &simplified.positions;
      triPtr = &simplified.triangles;

      std::cout << "  mip " << mipLevel << ": simplified "
                << mesh.triangles.size() << " -> " << simplified.triangles.size()
                << " tris (" << simplified.positions.size() << " verts)\n";
    }

    auto trisMip = *triPtr;
    SortTrianglesSpatially(trisMip, *posPtr);

    float lodError = meshDiagonal * (1 << mipLevel) * 0.001f;
    auto clusters =
        MakeClusters(trisMip, *posPtr, trianglesPerCluster, lodError);

    rawMips.push_back({mipLevel, std::move(clusters)});
  }

  // Split into pages (max maxClustersPerMip clusters per page)
  BuildResult result;
  result.mipLevels = mipValues;

  for (auto &raw : rawMips) {
    const size_t n = raw.clusters.size();
    for (size_t start = 0; start < n;
         start += static_cast<size_t>(maxClustersPerMip)) {
      const size_t end =
          std::min(start + static_cast<size_t>(maxClustersPerMip), n);

      ClusterPage page;
      page.mipLevel = raw.mipLevel;

      for (size_t i = start; i < end; ++i) {
        page.bounds.expand(raw.clusters[i].bounds);
        page.lodSphere =
            MergeSpheres(page.lodSphere, raw.clusters[i].lodSphere);
        page.clusters.push_back(std::move(raw.clusters[i]));
      }

      result.pages.push_back(std::move(page));
    }
  }

  if (result.pages.size() > kMaxTotalPages) {
    std::cout << "  Warning: " << result.pages.size()
              << " pages needed, trimming to " << kMaxTotalPages
              << " (coarsest mip levels dropped)\n";
    result.pages.resize(kMaxTotalPages);
  }

  return result;
}
