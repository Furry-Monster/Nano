#pragma once

#include "float4.h"

class matrix3 {
public:
  union {
    struct {
      float _11, _12, _13;
      float _21, _22, _23;
      float _31, _32, _33;
    };
    float v[9];
  };

  void LoadIdentity();
  void SetScale(float sx, float sy, float sz);
  void operator=(const matrix3 &rhs);
  matrix3 operator*(const matrix3 &rhs) const;
  void Transpose();
  float Determinant() const;
};

class matrix4 {
public:
  union {
    struct {
      float _11, _12, _13, _14;
      float _21, _22, _23, _24;
      float _31, _32, _33, _34;
      float _41, _42, _43, _44;
    };
    float v[16];
  };

  void LoadIdentity();
  void Perspective(float fovDegrees, float aspect, float nearPlane,
                   float farPlane);
  void LookAt(const float4 &eye, const float4 &target, const float4 &up);
  void SetLeftTop3x3(const matrix3 &m);
  void Translate(float tx, float ty, float tz);
  matrix3 GetMIJ(int row, int col) const;
  void Transpose();
  float Determinant() const;
  matrix4 Invert() const;
  matrix4 operator*(const matrix4 &rhs) const;
  void operator=(const matrix4 &rhs);
};

float4 operator*(const float4 &vec, const matrix4 &mat);
