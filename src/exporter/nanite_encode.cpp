#include "nanite_encode.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

// BVH layout: 52 u32s/node. [0..15] LODBounds, [16..31] Misc0, [32..47] Misc1,
// [48..51] Misc2. Matches NodeAndClusterCull.glsl.
namespace {

constexpr uint32_t kNodeU32Size = 52;
constexpr uint32_t kMaxFanout = 4;
constexpr uint32_t kInternalNodeCount = 5; // root + 4 children

inline uint32_t FB(float v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(float));
  return u;
}

struct BvhWriter {
  std::vector<uint32_t> &buf;

  void setLODBounds(uint32_t node, uint32_t child, const Sphere &s) const {
    uint32_t base = node * kNodeU32Size + child * 4;
    buf[base + 0] = FB(s.center.x);
    buf[base + 1] = FB(s.center.y);
    buf[base + 2] = FB(s.center.z);
    buf[base + 3] = FB(s.radius);
  }

  void setMisc0(uint32_t node, uint32_t child, const Vec3 &boxCenter,
                float minLodErr, float maxParentErr) const {
    uint32_t base = node * kNodeU32Size + 16 + child * 4;
    buf[base + 0] = FB(boxCenter.x);
    buf[base + 1] = FB(boxCenter.y);
    buf[base + 2] = FB(boxCenter.z);
    buf[base + 3] = PackHalf2x16(minLodErr, maxParentErr);
  }

  void setMisc1(uint32_t node, uint32_t child, const Vec3 &boxExtent,
                uint32_t childStartRef) const {
    uint32_t base = node * kNodeU32Size + 32 + child * 4;
    buf[base + 0] = FB(boxExtent.x);
    buf[base + 1] = FB(boxExtent.y);
    buf[base + 2] = FB(boxExtent.z);
    buf[base + 3] = childStartRef;
  }

  void setMisc2(uint32_t node, uint32_t child, uint32_t misc2) const {
    buf[node * kNodeU32Size + 48 + child] = misc2;
  }

  void setLeaf(uint32_t node, uint32_t child, const Sphere &lodSphere,
               const AABB &box, float minLodErr, float maxParentErr,
               uint32_t childStartRef, uint32_t misc2) const {
    setLODBounds(node, child, lodSphere);
    setMisc0(node, child, box.center(), minLodErr, maxParentErr);
    setMisc1(node, child, box.halfExtent(), childStartRef);
    setMisc2(node, child, misc2);
  }

  void setEmpty(uint32_t node, uint32_t child) const {
    setLODBounds(node, child, {{0, 0, 0}, 0.f});
    setMisc0(node, child, {0, 0, 0}, 0.f, 0.f);
    setMisc1(node, child, {0, 0, 0}, kChildRefInvalid);
    setMisc2(node, child, 0u);
  }
};

} // namespace

void EncodeBVH(const BuildResult &build, const std::string &outPath) {
  const uint32_t numPages = static_cast<uint32_t>(build.pages.size());
  if (numPages == 0)
    throw std::runtime_error("No pages to encode into BVH");

  const uint32_t maxPages = std::min(numPages, 16u);

  std::vector<uint32_t> bvh(kInternalNodeCount * kNodeU32Size, 0);
  BvhWriter wr{bvh};

  for (uint32_t parent = 0; parent < kMaxFanout; ++parent) {
    const uint32_t nodeIndex = 1 + parent;
    const uint32_t pageStart = parent * kMaxFanout;
    const uint32_t pageEnd = std::min(pageStart + kMaxFanout, maxPages);

    if (pageStart >= maxPages) {
      for (uint32_t c = 0; c < kMaxFanout; ++c)
        wr.setEmpty(nodeIndex, c);
      // Root's child: mark as internal (will traverse but find all empty).
      wr.setLeaf(0, parent, {{0, 0, 0}, 0.f}, AABB{}, 0.f, 0.f, nodeIndex,
                 kMisc2InternalNode);
      continue;
    }

    AABB groupBox;
    Sphere groupSphere;

    for (uint32_t c = 0; c < kMaxFanout; ++c) {
      const uint32_t pageIdx = pageStart + c;

      if (pageIdx >= pageEnd) {
        wr.setEmpty(nodeIndex, c);
        continue;
      }

      const ClusterPage &page = build.pages[pageIdx];
      const uint32_t clusterCount = static_cast<uint32_t>(page.clusters.size());
      if (clusterCount > 511u)
        throw std::runtime_error("Cluster count > 511 on page " +
                                 std::to_string(pageIdx));

      const uint32_t childStartRef = (pageIdx << 8) | 0u;
      const uint32_t misc2 =
          PackMisc2Leaf(clusterCount, static_cast<uint32_t>(page.mipLevel));

      wr.setLeaf(nodeIndex, c, page.lodSphere, page.bounds, 0.f, 1e10f,
                 childStartRef, misc2);

      groupBox.expand(page.bounds);
      groupSphere = MergeSpheres(groupSphere, page.lodSphere);
    }

    wr.setLeaf(0, parent, groupSphere, groupBox, 0.f, 1e10f, nodeIndex,
               kMisc2InternalNode);
  }

  WriteU32File(outPath, bvh);

  std::cout << "  BVH: " << kInternalNodeCount << " nodes, " << bvh.size() * 4
            << " bytes\n";
}

// NaniteMesh: [0] pageCount, [1..n] pageOffsets. Per page: clusterCount,
// clusterOffsets, cluster data. Per cluster: +0 indexOffset, +1 indexCount,
// +2..5 LODBounds, +6 lodError|edgeLength, +7.. positions, then indices.
void EncodeNaniteMesh(const BuildResult &build, uint32_t indexCountPerCluster,
                      const std::string &outPath) {
  const auto pageCount = static_cast<uint32_t>(build.pages.size());

  std::vector<uint32_t> out;
  out.reserve(4u * 1024u * 1024u);

  out.push_back(pageCount);
  out.resize(1 + pageCount, 0); // placeholder offsets

  // Running absolute byte offset (starts after header + page offset table).
  uint32_t absOffset = (1 + pageCount) * 4u;
  for (uint32_t pi = 0; pi < pageCount; ++pi) {
    const ClusterPage &page = build.pages[pi];
    const uint32_t clusterCount = static_cast<uint32_t>(page.clusters.size());

    out[1 + pi] = absOffset;
    if (clusterCount == 0)
      continue;

    out.push_back(clusterCount);

    const size_t clusterOffsetsStart = out.size();
    out.resize(out.size() + clusterCount, 0);
    uint32_t clusterRelOffset = 0;

    for (uint32_t ci = 0; ci < clusterCount; ++ci) {
      const Cluster &cl = page.clusters[ci];
      out[clusterOffsetsStart + ci] = clusterRelOffset;

      const uint32_t numVerts = static_cast<uint32_t>(cl.positions.size());
      const uint32_t numIdx = static_cast<uint32_t>(cl.indices.size());
      const uint32_t indexDataOffsetBytes = (7u + numVerts * 3u) * 4u;

      out.push_back(indexDataOffsetBytes);
      out.push_back(numIdx);
      out.push_back(FloatBitsToU32(cl.lodSphere.center.x));
      out.push_back(FloatBitsToU32(cl.lodSphere.center.y));
      out.push_back(FloatBitsToU32(cl.lodSphere.center.z));
      out.push_back(FloatBitsToU32(cl.lodSphere.radius));
      out.push_back(PackLodErrorEdgeLength(cl.lodError, cl.edgeLength));

      for (const Vec3 &p : cl.positions) {
        out.push_back(FloatBitsToU32(p.x));
        out.push_back(FloatBitsToU32(p.y));
        out.push_back(FloatBitsToU32(p.z));
      }

      const uint32_t copyCount = std::min(numIdx, indexCountPerCluster);
      for (uint32_t k = 0; k < copyCount; ++k)
        out.push_back(cl.indices[k]);
      for (uint32_t k = copyCount; k < indexCountPerCluster; ++k)
        out.push_back(0u);

      clusterRelOffset += (7u + numVerts * 3u + indexCountPerCluster) * 4u;
    }

    const uint32_t pageBytes = 4u + clusterCount * 4u + clusterRelOffset;
    absOffset += pageBytes;
  }

  {
    const uint32_t actualBytes = static_cast<uint32_t>(out.size()) * 4u;
    if (actualBytes != absOffset) {
      throw std::runtime_error("NaniteMesh size mismatch: written " +
                               std::to_string(actualBytes) +
                               " bytes, expected " + std::to_string(absOffset));
    }
  }

  for (uint32_t pi = 0; pi < pageCount; ++pi) {
    const uint32_t pageByteOff = out[1 + pi];
    if (pageByteOff % 4 != 0)
      throw std::runtime_error("Page offset not 4-aligned");
    const uint32_t pageBase = pageByteOff / 4;
    if (pageBase >= out.size())
      throw std::runtime_error("Page offset out of bounds");

    const uint32_t cc = out[pageBase];
    if (cc != static_cast<uint32_t>(build.pages[pi].clusters.size()))
      throw std::runtime_error("Cluster count mismatch in verification");

    for (uint32_t ci = 0; ci < cc; ++ci) {
      const uint32_t clOff = out[pageBase + 1 + ci];
      if (clOff % 4 != 0)
        throw std::runtime_error("Cluster offset not 4-aligned");

      const uint32_t clBase = pageBase + 1 + cc + clOff / 4;
      if (clBase + 7 > out.size())
        throw std::runtime_error("Cluster header out of bounds");

      const uint32_t idxOff = out[clBase]; // indexDataOffsetLocal
      if (idxOff % 4 != 0)
        throw std::runtime_error("Index data offset not 4-aligned");

      const uint32_t idxBase = clBase + idxOff / 4;
      if (idxBase + build.pages[pi].clusters[ci].indices.size() > out.size())
        throw std::runtime_error("Index data out of bounds for page " +
                                 std::to_string(pi) + " cluster " +
                                 std::to_string(ci));
    }
  }

  WriteU32File(outPath, out);

  std::cout << "  NaniteMesh: " << pageCount << " pages, " << out.size() * 4
            << " bytes\n";
}
