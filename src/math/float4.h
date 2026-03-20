#pragma once

#include <cmath>
#include <cstring>

class float4 {
public:
  union {
    struct {
      float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};
    };
    float v[4];
  };

  float4() = default;
  explicit float4(float val) : x(val), y(val), z(val), w(val) {}
  explicit float4(float *data)
      : x(data[0]), y(data[1]), z(data[2]), w(data[3]) {}
  float4(float ix, float iy, float iz, float iw = 1.0f)
      : x(ix), y(iy), z(iz), w(iw) {}
  float4(const float4 &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

  float &operator[](int index) { return v[index]; }

  void operator=(const float4 &rhs);
  void operator*=(float scalar);
  void operator-=(float scalar);
  void operator/=(float scalar);
  void operator+=(float scalar);

  float4 operator+(const float4 &rhs) const;
  float4 operator-(const float4 &rhs) const;
  float4 operator*(float scalar) const;

  void Normalize();
};

float MinValue(float a, float b);
float MaxValue(float a, float b);
float4 Min(const float4 &a, const float4 &b);
float4 Max(const float4 &a, const float4 &b);
float4 cross(const float4 &a, const float4 &b);
float dot3(const float4 &a, const float4 &b);
float dot4(const float4 &a, const float4 &b);
