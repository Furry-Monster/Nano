#include "cluster_builder.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kEpsilon = 1e-9f;
constexpr int kGridRes = 8; // 8^3 = 512 cells for spatial stratification

// Decimate with spatial stratification: sample proportionally from each
// grid cell so coarse LODs cover the entire mesh, not just one region.
std::vector<Triangle>
DecimateTrianglesSpatial(const std::vector<Triangle> &tris,
                         const std::vector<Vec3> &positions, uint64_t step,
                         Vec3 meshMin, Vec3 meshMax) {
  if (step <= 1)
    return tris;
  const size_t targetCount = (tris.size() + step - 1) / step;
  if (targetCount >= tris.size())
    return tris;

  const float dx = meshMax.x - meshMin.x;
  const float dy = meshMax.y - meshMin.y;
  const float dz = meshMax.z - meshMin.z;
  const float scaleX = (dx > kEpsilon) ? (kGridRes - 1) / dx : 0.f;
  const float scaleY = (dy > kEpsilon) ? (kGridRes - 1) / dy : 0.f;
  const float scaleZ = (dz > kEpsilon) ? (kGridRes - 1) / dz : 0.f;

  std::vector<std::vector<size_t>> cells(kGridRes * kGridRes * kGridRes);
  for (size_t i = 0; i < tris.size(); ++i) {
    const Triangle &t = tris[i];
    float cx = (positions[t.a].x + positions[t.b].x + positions[t.c].x) / 3.f;
    float cy = (positions[t.a].y + positions[t.b].y + positions[t.c].y) / 3.f;
    float cz = (positions[t.a].z + positions[t.b].z + positions[t.c].z) / 3.f;
    int gx = static_cast<int>((cx - meshMin.x) * scaleX);
    int gy = static_cast<int>((cy - meshMin.y) * scaleY);
    int gz = static_cast<int>((cz - meshMin.z) * scaleZ);
    gx = std::clamp(gx, 0, kGridRes - 1);
    gy = std::clamp(gy, 0, kGridRes - 1);
    gz = std::clamp(gz, 0, kGridRes - 1);
    size_t cellIdx = gz * (kGridRes * kGridRes) + gy * kGridRes + gx;
    cells[cellIdx].push_back(i);
  }

  std::vector<Triangle> out;
  out.reserve(targetCount);
  const size_t total = tris.size();
  for (auto &cell : cells) {
    if (cell.empty() || out.size() >= targetCount)
      continue;
    const size_t want = (cell.size() * targetCount + total - 1) / total;
    const size_t n = std::min({want, cell.size(), targetCount - out.size()});
    for (size_t j = 0; j < n; ++j) {
      out.push_back(tris[cell[j]]);
    }
  }
  if (out.empty() && !tris.empty())
    out.push_back(tris[0]);
  return out;
}

float EdgeLength(const Vec3 &a, const Vec3 &b) {
  float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

BuildResult BuildClustersAndPages(const LoadedMesh &mesh,
                                  const std::vector<int> &mipValues,
                                  uint32_t trianglesPerCluster,
                                  uint32_t maxClustersPerMip) {
  if (mesh.triangles.empty() || mesh.positions.empty()) {
    throw std::runtime_error("Empty mesh");
  }
  if (trianglesPerCluster == 0) {
    throw std::runtime_error("trianglesPerCluster must be > 0");
  }

  Vec3 meshMin{1e30f, 1e30f, 1e30f}, meshMax{-1e30f, -1e30f, -1e30f};
  for (const auto &p : mesh.positions) {
    meshMin.x = std::min(meshMin.x, p.x);
    meshMin.y = std::min(meshMin.y, p.y);
    meshMin.z = std::min(meshMin.z, p.z);
    meshMax.x = std::max(meshMax.x, p.x);
    meshMax.y = std::max(meshMax.y, p.y);
    meshMax.z = std::max(meshMax.z, p.z);
  }

  BuildResult result;
  result.mipLevels = mipValues;

  for (size_t mipIdx = 0; mipIdx < mipValues.size(); ++mipIdx) {
    const int mipLevel = mipValues[mipIdx];
    const uint64_t step = static_cast<uint64_t>(std::max(1, 1 << mipLevel));
    auto trisMip = DecimateTrianglesSpatial(mesh.triangles, mesh.positions,
                                            step, meshMin, meshMax);

    if (trisMip.empty()) {
      trisMip.push_back(mesh.triangles[0]);
    }

    const uint64_t maxTris = static_cast<uint64_t>(maxClustersPerMip) *
                             static_cast<uint64_t>(trianglesPerCluster);
    if (trisMip.size() > maxTris) {
      const uint64_t extraStep = (trisMip.size() + maxTris - 1) / maxTris;
      trisMip = DecimateTrianglesSpatial(trisMip, mesh.positions, extraStep,
                                         meshMin, meshMax);
    }

    ClusterPage page;
    page.mipLevel = mipLevel;

    std::vector<int32_t> globalToLocal(mesh.positions.size(), -1);
    std::vector<uint32_t> usedVertices;
    usedVertices.reserve(trianglesPerCluster * 3);

    const size_t numClusters =
        (trisMip.size() + trianglesPerCluster - 1) / trianglesPerCluster;

    for (size_t ci = 0; ci < numClusters; ++ci) {
      const size_t triStart = ci * trianglesPerCluster;
      const size_t triEnd =
          std::min(triStart + trianglesPerCluster, trisMip.size());

      Cluster cl;
      cl.positions.clear();
      cl.indices.clear();
      globalToLocal.assign(mesh.positions.size(), -1);
      usedVertices.clear();

      Vec3 bmin{1e30f, 1e30f, 1e30f};
      Vec3 bmax{-1e30f, -1e30f, -1e30f};
      float maxEdge = 0.f;

      for (size_t ti = triStart; ti < triEnd; ++ti) {
        const Triangle &t = trisMip[ti];
        const uint32_t ids[3] = {t.a, t.b, t.c};
        uint32_t localIds[3];

        for (int k = 0; k < 3; ++k) {
          const uint32_t gid = ids[k];
          if (globalToLocal[gid] < 0) {
            const int32_t localId = static_cast<int32_t>(cl.positions.size());
            globalToLocal[gid] = localId;
            usedVertices.push_back(gid);
            const Vec3 &p = mesh.positions[gid];
            cl.positions.push_back(p);
            bmin.x = std::min(bmin.x, p.x);
            bmin.y = std::min(bmin.y, p.y);
            bmin.z = std::min(bmin.z, p.z);
            bmax.x = std::max(bmax.x, p.x);
            bmax.y = std::max(bmax.y, p.y);
            bmax.z = std::max(bmax.z, p.z);
          }
          localIds[k] = static_cast<uint32_t>(globalToLocal[gid]);
        }

        cl.indices.push_back(localIds[0]);
        cl.indices.push_back(localIds[1]);
        cl.indices.push_back(localIds[2]);

        const Vec3 &a = cl.positions[localIds[0]];
        const Vec3 &b = cl.positions[localIds[1]];
        const Vec3 &c = cl.positions[localIds[2]];
        maxEdge = std::max(maxEdge, EdgeLength(a, b));
        maxEdge = std::max(maxEdge, EdgeLength(b, c));
        maxEdge = std::max(maxEdge, EdgeLength(c, a));
      }

      cl.boundsMin = bmin;
      cl.boundsMax = bmax;
      cl.edgeLength = maxEdge;

      // LOD error: approximate screen-space error. Use cluster extent /
      // decimation factor.
      const float extent = std::max(
          {bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z, kEpsilon});
      cl.lodError = extent * static_cast<float>(step) * 0.5f;

      page.clusters.push_back(std::move(cl));
    }

    result.pages.push_back(std::move(page));
  }

  return result;
}
