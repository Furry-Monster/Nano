#pragma once

#include "matrix4.h"

class quaternion {
public:
  float w, x, y, z;

  quaternion(float angleX, float angleY, float angleZ);
  quaternion(float w, float x, float y, float z);

  void operator=(const quaternion &rhs);
  quaternion operator*(const quaternion &rhs) const;
  matrix3 toMatrix3() const;
};
