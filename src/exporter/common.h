#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Vec3 {
  float x = 0.f, y = 0.f, z = 0.f;

  Vec3() = default;
  Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

  Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

  float dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
  float lengthSq() const { return dot(*this); }
  float length() const { return std::sqrt(lengthSq()); }
};

inline Vec3 VecMin(const Vec3 &a, const Vec3 &b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

inline Vec3 VecMax(const Vec3 &a, const Vec3 &b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

struct Triangle {
  uint32_t a = 0, b = 0, c = 0;
};

struct AABB {
  Vec3 lo{1e30f, 1e30f, 1e30f};
  Vec3 hi{-1e30f, -1e30f, -1e30f};

  void expand(const Vec3 &p) {
    lo = VecMin(lo, p);
    hi = VecMax(hi, p);
  }
  void expand(const AABB &o) {
    lo = VecMin(lo, o.lo);
    hi = VecMax(hi, o.hi);
  }
  Vec3 center() const { return (lo + hi) * 0.5f; }
  Vec3 halfExtent() const { return (hi - lo) * 0.5f; }
  float diagonal() const { return (hi - lo).length(); }
  bool valid() const { return lo.x <= hi.x && lo.y <= hi.y && lo.z <= hi.z; }
};

struct Sphere {
  Vec3 center{0, 0, 0};
  float radius = 0.f;
};

inline Sphere ComputeBoundingSphere(const Vec3 *pts, size_t n) {
  if (n == 0)
    return {};
  AABB box;
  for (size_t i = 0; i < n; ++i)
    box.expand(pts[i]);
  Vec3 c = box.center();
  float maxDistSq = 0.f;
  for (size_t i = 0; i < n; ++i)
    maxDistSq = std::max(maxDistSq, (pts[i] - c).lengthSq());
  return {c, std::sqrt(maxDistSq)};
}

inline Sphere MergeSpheres(const Sphere &a, const Sphere &b) {
  if (a.radius <= 0.f)
    return b;
  if (b.radius <= 0.f)
    return a;
  Vec3 d = b.center - a.center;
  float dist = d.length();
  if (dist + b.radius <= a.radius)
    return a;
  if (dist + a.radius <= b.radius)
    return b;
  float newRadius = (dist + a.radius + b.radius) * 0.5f;
  float t = (newRadius - a.radius) / std::max(dist, 1e-9f);
  Vec3 newCenter = a.center + d * t;
  return {newCenter, newRadius};
}

inline uint32_t FloatBitsToU32(float v) {
  uint32_t out;
  std::memcpy(&out, &v, sizeof(uint32_t));
  return out;
}

// IEEE 754 binary16, matches GLSL unpackHalf2x16
inline uint16_t FloatToHalf(float v) {
  uint32_t u;
  std::memcpy(&u, &v, sizeof(float));
  const uint32_t sign = (u >> 16) & 0x8000u;
  const int32_t exp = static_cast<int32_t>((u >> 23) & 0xFFu) - 127;
  const uint32_t mantissa = u & 0x7FFFFFu;

  if (exp == 128) // Inf / NaN
    return static_cast<uint16_t>(sign | 0x7C00u | (mantissa ? 0x200u : 0u));
  if (exp < -24) // underflow to zero
    return static_cast<uint16_t>(sign);
  if (exp < -14) { // denormalized half
    uint32_t m = mantissa | 0x800000u;
    int shift = -14 - exp + 13;
    return static_cast<uint16_t>(sign | (m >> shift));
  }
  if (exp > 15) // overflow to inf
    return static_cast<uint16_t>(sign | 0x7C00u);
  return static_cast<uint16_t>(sign | ((exp + 15) << 10) | (mantissa >> 13));
}

inline uint32_t PackHalf2x16(float lo, float hi) {
  return static_cast<uint32_t>(FloatToHalf(lo)) |
         (static_cast<uint32_t>(FloatToHalf(hi)) << 16);
}

inline uint32_t PackLodErrorEdgeLength(float lodError, float edgeLength) {
  return PackHalf2x16(lodError, edgeLength);
}

// Misc2 layout (match NodeAndClusterCull.glsl): [0:8] NumChildren, [9:13] MipLevel
inline uint32_t PackMisc2Leaf(uint32_t numChildren, uint32_t mipLevel) {
  return (numChildren & 0x1FFu) | ((mipLevel & 0x1Fu) << 9);
}

constexpr uint32_t kMisc2InternalNode = 0xFFFFFFFFu;
constexpr uint32_t kChildRefInvalid = 0xFFFFFFFFu;

inline void WriteU32File(const std::string &path,
                         const std::vector<uint32_t> &data) {
  std::ofstream f(path, std::ios::binary);
  if (!f)
    throw std::runtime_error("Cannot open output: " + path);
  f.write(reinterpret_cast<const char *>(data.data()),
          static_cast<std::streamsize>(data.size() * sizeof(uint32_t)));
}
