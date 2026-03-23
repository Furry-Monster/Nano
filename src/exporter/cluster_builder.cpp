#include "cluster_builder.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kEpsilon = 1e-9f;

float EdgeLen(const Vec3 &a, const Vec3 &b) { return (b - a).length(); }

// Uniform stride decimation: keep every step-th triangle, ensuring uniform
// coverage over the original index order (which Assimp typically lays out in
// spatial locality). This avoids the bias of spatial-grid stratification where
// cells with few triangles get disproportionate weight.
std::vector<Triangle> DecimateTris(const std::vector<Triangle> &tris,
                                   uint64_t step) {
  if (step <= 1)
    return tris;
  std::vector<Triangle> out;
  const size_t n = tris.size();
  out.reserve((n + step - 1) / step);
  for (size_t i = 0; i < n; i += step)
    out.push_back(tris[i]);
  if (out.empty() && !tris.empty())
    out.push_back(tris[0]);
  return out;
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

  BuildResult result;
  result.mipLevels = mipValues;

  for (size_t mipIdx = 0; mipIdx < mipValues.size(); ++mipIdx) {
    const int mipLevel = mipValues[mipIdx];
    const uint64_t step =
        static_cast<uint64_t>(std::max(1, 1 << std::min(mipLevel, 20)));
    auto trisMip = DecimateTris(mesh.triangles, step);

    // Secondary decimation if still exceeding the per-mip cap.
    const uint64_t maxTris = static_cast<uint64_t>(maxClustersPerMip) *
                             static_cast<uint64_t>(trianglesPerCluster);
    if (trisMip.size() > maxTris) {
      const uint64_t extra = (trisMip.size() + maxTris - 1) / maxTris;
      trisMip = DecimateTris(trisMip, extra);
    }

    ClusterPage page;
    page.mipLevel = mipLevel;

    // Per-cluster local vertex pool.  globalToLocal is reset per cluster.
    std::vector<int32_t> globalToLocal(mesh.positions.size(), -1);
    std::vector<uint32_t> usedVerts;
    usedVerts.reserve(trianglesPerCluster * 3);

    const size_t numClusters =
        (trisMip.size() + trianglesPerCluster - 1) / trianglesPerCluster;

    for (size_t ci = 0; ci < numClusters; ++ci) {
      const size_t triStart = ci * trianglesPerCluster;
      const size_t triEnd =
          std::min(triStart + trianglesPerCluster, trisMip.size());

      Cluster cl;
      globalToLocal.assign(mesh.positions.size(), -1);
      usedVerts.clear();

      AABB box;
      float maxEdge = 0.f;

      for (size_t ti = triStart; ti < triEnd; ++ti) {
        const Triangle &t = trisMip[ti];
        const uint32_t ids[3] = {t.a, t.b, t.c};
        uint32_t localIds[3];

        for (int k = 0; k < 3; ++k) {
          const uint32_t gid = ids[k];
          if (globalToLocal[gid] < 0) {
            globalToLocal[gid] = static_cast<int32_t>(cl.positions.size());
            usedVerts.push_back(gid);
            const Vec3 &p = mesh.positions[gid];
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

      cl.bounds = box;
      cl.lodSphere =
          ComputeBoundingSphere(cl.positions.data(), cl.positions.size());
      cl.edgeLength = maxEdge;

      const float extent = std::max({box.hi.x - box.lo.x, box.hi.y - box.lo.y,
                                     box.hi.z - box.lo.z, kEpsilon});
      cl.lodError = extent * static_cast<float>(step) * 0.5f;

      page.bounds.expand(cl.bounds);
      page.lodSphere = MergeSpheres(page.lodSphere, cl.lodSphere);

      page.clusters.push_back(std::move(cl));
    }

    result.pages.push_back(std::move(page));
  }

  return result;
}
