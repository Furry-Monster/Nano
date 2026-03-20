#include "float4.h"

void float4::operator=(const float4 &rhs) { std::memcpy(v, rhs.v, sizeof(v)); }

void float4::operator*=(float scalar) {
  x *= scalar;
  y *= scalar;
  z *= scalar;
}

void float4::operator-=(float scalar) {
  x -= scalar;
  y -= scalar;
  z -= scalar;
}

void float4::operator/=(float scalar) {
  x /= scalar;
  y /= scalar;
  z /= scalar;
}

void float4::operator+=(float scalar) {
  x += scalar;
  y += scalar;
  z += scalar;
}

float4 float4::operator+(const float4 &rhs) const {
  return float4(x + rhs.x, y + rhs.y, z + rhs.z, w);
}

float4 float4::operator-(const float4 &rhs) const {
  return float4(x - rhs.x, y - rhs.y, z - rhs.z, w);
}

float4 float4::operator*(float scalar) const {
  return float4(x * scalar, y * scalar, z * scalar, w);
}

void float4::Normalize() {
  float magnitude = std::sqrt(x * x + y * y + z * z);
  if (magnitude < 0.000001f)
    return;
  x /= magnitude;
  y /= magnitude;
  z /= magnitude;
}

float MinValue(float a, float b) { return a < b ? a : b; }
float MaxValue(float a, float b) { return a > b ? a : b; }

float4 Min(const float4 &a, const float4 &b) {
  return float4(MinValue(a.x, b.x), MinValue(a.y, b.y), MinValue(a.z, b.z),
                MinValue(a.w, b.w));
}

float4 Max(const float4 &a, const float4 &b) {
  return float4(MaxValue(a.x, b.x), MaxValue(a.y, b.y), MaxValue(a.z, b.z),
                MaxValue(a.w, b.w));
}

float4 cross(const float4 &a, const float4 &b) {
  return float4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x, a.w);
}

float dot3(const float4 &a, const float4 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float dot4(const float4 &a, const float4 &b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
