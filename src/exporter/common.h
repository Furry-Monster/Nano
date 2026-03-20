#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct Triangle {
  uint32_t a = 0;
  uint32_t b = 0;
  uint32_t c = 0;
};

inline uint32_t FloatToU32(float v) {
  uint32_t out = 0;
  std::memcpy(&out, &v, sizeof(uint32_t));
  return out;
}

inline uint32_t PackMisc2(uint32_t numChildren, uint32_t numPages,
                           uint32_t startPageIndex) {
  // Must match NodeAndClusterCull.glsl decoding.
  // NumChildren: bits [0..8]
  // NumPages:    bits [9..13]
  // StartPageIndex: bits [14..29]
  constexpr uint32_t kNumChildrenBits = 9;
  constexpr uint32_t kNumPagesBits = 5;
  constexpr uint32_t kStartPageIndexBits = 16;
  constexpr uint32_t kNumChildrenMask = (1u << kNumChildrenBits) - 1u;
  constexpr uint32_t kNumPagesMask = (1u << kNumPagesBits) - 1u;
  constexpr uint32_t kStartPageIndexMask = (1u << kStartPageIndexBits) - 1u;

  uint32_t misc2 = 0;
  misc2 |= (numChildren & kNumChildrenMask);
  misc2 |= ((numPages & kNumPagesMask) << kNumChildrenBits);
  misc2 |=
      ((startPageIndex & kStartPageIndexMask)
       << (kNumChildrenBits + kNumPagesBits));
  return misc2;
}

inline void WriteU32File(const std::string& path,
                          const std::vector<uint32_t>& u32) {
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Failed to open output file: " + path);
  }
  f.write(reinterpret_cast<const char*>(u32.data()),
          static_cast<std::streamsize>(u32.size() * sizeof(uint32_t)));
}

struct ExportCluster {
  std::vector<Vec3> positions; // local unique vertices
  std::vector<uint32_t> indexRemap; // size = indexCount
};

struct ExportPage {
  std::vector<ExportCluster> clusters;
};

