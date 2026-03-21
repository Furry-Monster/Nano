#pragma once

#include "matrix4.h"

class quaternion {
public:
  float w, x, y, z;

  quaternion(float angleX, float angleY, float angleZ);
  quaternion(float w, float x, float y, float z);

  quaternion(const quaternion &other);
  quaternion(quaternion &&other) noexcept;

  quaternion &operator=(const quaternion &rhs);
  quaternion &operator=(quaternion &&rhs) noexcept;

  quaternion operator*(const quaternion &rhs) const;
  matrix3 toMatrix3() const;
};
