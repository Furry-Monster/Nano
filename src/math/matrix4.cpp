#include "matrix4.h"

#include <cmath>
#include <cstring>

void matrix3::LoadIdentity() {
  std::memset(v, 0, sizeof(v));
  _11 = 1.0f;
  _22 = 1.0f;
  _33 = 1.0f;
}

void matrix3::SetScale(float sx, float sy, float sz) {
  _11 = sx;
  _22 = sy;
  _33 = sz;
}

void matrix3::operator=(const matrix3 &rhs) {
  std::memcpy(v, rhs.v, sizeof(v));
}

matrix3 matrix3::operator*(const matrix3 &rhs) const {
  matrix3 ret;
  ret._11 = _11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31;
  ret._12 = _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32;
  ret._13 = _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33;

  ret._21 = _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31;
  ret._22 = _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32;
  ret._23 = _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33;

  ret._31 = _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31;
  ret._32 = _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32;
  ret._33 = _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33;
  return ret;
}

void matrix3::Transpose() {
  std::swap(_12, _21);
  std::swap(_13, _31);
  std::swap(_23, _32);
}

float matrix3::Determinant() const {
  return _11 * (_22 * _33 - _32 * _23) - _12 * (_21 * _33 - _31 * _23) +
         _13 * (_21 * _32 - _31 * _22);
}

void matrix4::Perspective(float fovDegrees, float aspect, float nearPlane,
                          float farPlane) {
  std::memset(v, 0, sizeof(v));
  // Half-FOV in radians, horizontal FOV
  float halfRad = fovDegrees * 0.5f * 3.1415926f / 180.0f;
  float R = std::tan(halfRad) * nearPlane;
  float T = R / aspect;
  _11 = nearPlane / R;
  _22 = nearPlane / T;
  _33 = (nearPlane + farPlane) / (nearPlane - farPlane);
  _43 = (2.0f * nearPlane * farPlane) / (nearPlane - farPlane);
  _34 = -1.0f;
}

void matrix4::LookAt(const float4 &eye, const float4 &target,
                     const float4 &up) {
  float4 z = eye - target;
  z.Normalize();
  float4 x = cross(up, z);
  x.Normalize();
  float4 y = cross(z, x);
  y.Normalize();

  matrix4 rotateMatrix;
  rotateMatrix.LoadIdentity();
  rotateMatrix._11 = x.x;
  rotateMatrix._12 = x.y;
  rotateMatrix._13 = x.z;
  rotateMatrix._21 = y.x;
  rotateMatrix._22 = y.y;
  rotateMatrix._23 = y.z;
  rotateMatrix._31 = z.x;
  rotateMatrix._32 = z.y;
  rotateMatrix._33 = z.z;

  matrix4 translateMatrix;
  translateMatrix.LoadIdentity();
  translateMatrix.Translate(eye.x, eye.y, eye.z);

  *this = rotateMatrix * translateMatrix;
  *this = Invert();
}

void matrix4::LoadIdentity() {
  std::memset(v, 0, sizeof(v));
  _11 = 1.0f;
  _22 = 1.0f;
  _33 = 1.0f;
  _44 = 1.0f;
}

void matrix4::SetLeftTop3x3(const matrix3 &m) {
  _11 = m._11;
  _12 = m._12;
  _13 = m._13;
  _21 = m._21;
  _22 = m._22;
  _23 = m._23;
  _31 = m._31;
  _32 = m._32;
  _33 = m._33;
}

void matrix4::Translate(float tx, float ty, float tz) {
  _41 = tx;
  _42 = ty;
  _43 = tz;
}

matrix3 matrix4::GetMIJ(int row, int col) const {
  matrix3 ret;
  int i = 0;
  for (int r = 1; r < 5; r++) {
    for (int c = 1; c < 5; c++) {
      if (r != row && c != col) {
        ret.v[i++] = v[(r - 1) * 4 + c - 1];
      }
    }
  }
  return ret;
}

void matrix4::Transpose() {
  std::swap(_12, _21);
  std::swap(_13, _31);
  std::swap(_14, _41);
  std::swap(_23, _32);
  std::swap(_24, _42);
  std::swap(_34, _43);
}

float matrix4::Determinant() const {
  return _11 * GetMIJ(1, 1).Determinant() - _12 * GetMIJ(1, 2).Determinant() +
         _13 * GetMIJ(1, 3).Determinant() - _14 * GetMIJ(1, 4).Determinant();
}

matrix4 matrix4::Invert() const {
  matrix4 ret;
  float det = Determinant();
  float absDet = det > 0.0f ? det : -det;
  if (absDet < 0.000001f) {
    ret.LoadIdentity();
    return ret;
  }

  ret._11 = GetMIJ(1, 1).Determinant();
  ret._12 = -GetMIJ(1, 2).Determinant();
  ret._13 = GetMIJ(1, 3).Determinant();
  ret._14 = -GetMIJ(1, 4).Determinant();
  ret._21 = -GetMIJ(2, 1).Determinant();
  ret._22 = GetMIJ(2, 2).Determinant();
  ret._23 = -GetMIJ(2, 3).Determinant();
  ret._24 = GetMIJ(2, 4).Determinant();
  ret._31 = GetMIJ(3, 1).Determinant();
  ret._32 = -GetMIJ(3, 2).Determinant();
  ret._33 = GetMIJ(3, 3).Determinant();
  ret._34 = -GetMIJ(3, 4).Determinant();
  ret._41 = -GetMIJ(4, 1).Determinant();
  ret._42 = GetMIJ(4, 2).Determinant();
  ret._43 = -GetMIJ(4, 3).Determinant();
  ret._44 = GetMIJ(4, 4).Determinant();

  ret.Transpose();
  float invDet = 1.0f / det;
  for (int i = 0; i < 16; i++) {
    ret.v[i] *= invDet;
  }
  return ret;
}

matrix4 matrix4::operator*(const matrix4 &m) const {
  matrix4 ret;
  ret._11 =
      dot4(float4(_11, _12, _13, _14), float4(m._11, m._21, m._31, m._41));
  ret._12 =
      dot4(float4(_11, _12, _13, _14), float4(m._12, m._22, m._32, m._42));
  ret._13 =
      dot4(float4(_11, _12, _13, _14), float4(m._13, m._23, m._33, m._43));
  ret._14 =
      dot4(float4(_11, _12, _13, _14), float4(m._14, m._24, m._34, m._44));

  ret._21 =
      dot4(float4(_21, _22, _23, _24), float4(m._11, m._21, m._31, m._41));
  ret._22 =
      dot4(float4(_21, _22, _23, _24), float4(m._12, m._22, m._32, m._42));
  ret._23 =
      dot4(float4(_21, _22, _23, _24), float4(m._13, m._23, m._33, m._43));
  ret._24 =
      dot4(float4(_21, _22, _23, _24), float4(m._14, m._24, m._34, m._44));

  ret._31 =
      dot4(float4(_31, _32, _33, _34), float4(m._11, m._21, m._31, m._41));
  ret._32 =
      dot4(float4(_31, _32, _33, _34), float4(m._12, m._22, m._32, m._42));
  ret._33 =
      dot4(float4(_31, _32, _33, _34), float4(m._13, m._23, m._33, m._43));
  ret._34 =
      dot4(float4(_31, _32, _33, _34), float4(m._14, m._24, m._34, m._44));

  ret._41 =
      dot4(float4(_41, _42, _43, _44), float4(m._11, m._21, m._31, m._41));
  ret._42 =
      dot4(float4(_41, _42, _43, _44), float4(m._12, m._22, m._32, m._42));
  ret._43 =
      dot4(float4(_41, _42, _43, _44), float4(m._13, m._23, m._33, m._43));
  ret._44 =
      dot4(float4(_41, _42, _43, _44), float4(m._14, m._24, m._34, m._44));
  return ret;
}

void matrix4::operator=(const matrix4 &rhs) {
  std::memcpy(v, rhs.v, sizeof(v));
}

float4 operator*(const float4 &vec, const matrix4 &m) {
  float4 p;
  p.x = vec.x * m._11 + vec.y * m._21 + vec.z * m._31 + vec.w * m._41;
  p.y = vec.x * m._12 + vec.y * m._22 + vec.z * m._32 + vec.w * m._42;
  p.z = vec.x * m._13 + vec.y * m._23 + vec.z * m._33 + vec.w * m._43;
  p.w = vec.x * m._14 + vec.y * m._24 + vec.z * m._34 + vec.w * m._44;
  return p;
}
