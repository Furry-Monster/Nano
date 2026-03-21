#include "quaternion.h"

#include <cmath>

quaternion::quaternion(float angleX, float angleY, float angleZ) {
  float xRad = 0.5f * angleX * 3.1415926f / 180.0f;
  float yRad = 0.5f * angleY * 3.1415926f / 180.0f;
  float zRad = 0.5f * angleZ * 3.1415926f / 180.0f;

  quaternion qx(std::cos(xRad), std::sin(xRad), 0.0f, 0.0f);
  quaternion qy(std::cos(yRad), 0.0f, std::sin(yRad), 0.0f);
  quaternion qz(std::cos(zRad), 0.0f, 0.0f, std::sin(zRad));
  *this = (qx * qy) * qz;
}

quaternion::quaternion(float w, float x, float y, float z)
    : w(w), x(x), y(y), z(z) {}

quaternion::quaternion(const quaternion &other) {
  w = other.w;
  x = other.x;
  y = other.y;
  z = other.z;
}

quaternion::quaternion(quaternion &&other) noexcept {
  w = other.w;
  x = other.x;
  y = other.y;
  z = other.z;
}

quaternion &quaternion::operator=(const quaternion &rhs) {
  if (this == &rhs)
    return *this;

  w = rhs.w;
  x = rhs.x;
  y = rhs.y;
  z = rhs.z;

  return *this;
}

quaternion &quaternion::operator=(quaternion &&rhs) noexcept {
  if (this == &rhs)
    return *this;

  w = rhs.w;
  x = rhs.x;
  y = rhs.y;
  z = rhs.z;

  return *this;
}

quaternion quaternion::operator*(const quaternion &rhs) const {
  return quaternion(w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
                    w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                    w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
                    w * rhs.z + x * rhs.y + z * rhs.w - y * rhs.x);
}

matrix3 quaternion::toMatrix3() const {
  float xx = x * x, yy = y * y, zz = z * z;
  float xy = x * y, xz = x * z, yz = y * z;
  float wx = w * x, wy = w * y, wz = w * z;

  matrix3 m;
  m._11 = 1.0f - 2.0f * yy - 2.0f * zz;
  m._12 = 2.0f * xy + 2.0f * wz;
  m._13 = 2.0f * xz - 2.0f * wy;

  m._21 = 2.0f * xy - 2.0f * wz;
  m._22 = 1.0f - 2.0f * xx - 2.0f * zz;
  m._23 = 2.0f * yz + 2.0f * wx;

  m._31 = 2.0f * xz + 2.0f * wy;
  m._32 = 2.0f * yz + 2.0f * wx;
  m._33 = 1.0f - 2.0f * xx - 2.0f * yy;
  return m;
}
