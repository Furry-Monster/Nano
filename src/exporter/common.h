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

// Pack float to half (16-bit) for LODError/EdgeLength. Matches GLSL
// unpackHalf2x16.
inline uint16_t FloatToHalf(float v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(uint32_t));
  const uint32_t sign = (u >> 16) & 0x8000u;
  const uint32_t exp_mant = u & 0x7FFFFFFFu;
  if (exp_mant >= 0x7F800000u)
    return static_cast<uint16_t>(sign |
                                 (exp_mant > 0x7F800000u ? 0x7E00u : 0x7C00u));
  if (exp_mant < 0x38800000u)
    return static_cast<uint16_t>(sign);
  const uint32_t shifted = exp_mant - 0x38000000u + (0x1000u << 13);
  return static_cast<uint16_t>(sign | (shifted >> 13));
}

inline uint32_t PackLodErrorEdgeLength(float lodError, float edgeLength) {
  return static_cast<uint32_t>(FloatToHalf(lodError)) |
         (static_cast<uint32_t>(FloatToHalf(edgeLength)) << 16);
}

// Misc2 for BVH leaf: NumChildren (9 bits), NumPages/MipLevel (5 bits),
// StartPageIndex (16 bits). Must match NodeAndClusterCull.glsl
// BitFieldExtractU32 decoding.
inline uint32_t PackMisc2Leaf(uint32_t numChildren, uint32_t mipLevel,
                              uint32_t /*startPageIndex unused for leaf*/) {
  constexpr uint32_t kNumChildrenBits = 9;
  constexpr uint32_t kNumPagesBits = 5;
  constexpr uint32_t kNumChildrenMask = (1u << kNumChildrenBits) - 1u;
  constexpr uint32_t kNumPagesMask = (1u << kNumPagesBits) - 1u;
  return (numChildren & kNumChildrenMask) |
         ((mipLevel & kNumPagesMask) << kNumChildrenBits);
}

inline void WriteU32File(const std::string &path,
                         const std::vector<uint32_t> &u32) {
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Failed to open output file: " + path);
  }
  f.write(reinterpret_cast<const char *>(u32.data()),
          static_cast<std::streamsize>(u32.size() * sizeof(uint32_t)));
}

inline void WriteBinaryFile(const std::string &path, const void *data,
                            size_t size) {
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Failed to open output file: " + path);
  }
  f.write(reinterpret_cast<const char *>(data),
          static_cast<std::streamsize>(size));
}
