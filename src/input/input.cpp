#include "input/input.h"

#ifndef __ANDROID__
#include <GLFW/glfw3.h>
#endif

#include <algorithm>
#include <cmath>

#ifndef __ANDROID__
static GLFWwindow *sWindow = nullptr;
#endif
static float4 sCameraPosition(-330.f, 330.f, -330.f);
static float sYawRadians = 0.f;
static float sPitchRadians = 0.f;
static double sLastMouseX = 0.0;
static double sLastMouseY = 0.0;
static bool sFirstMouse = true;
#ifndef __ANDROID__
static bool sMouseLookEnabled = true;
#endif
static bool sCameraMovedThisFrame = false;
static constexpr float kMoveSpeed = 220.f;
static constexpr float kMouseSensitivity = 0.0022f;

static float4 CameraForwardFromAngles() {
  const float cosp = std::cos(sPitchRadians);
  float4 f(std::sin(sYawRadians) * cosp, std::sin(sPitchRadians),
           std::cos(sYawRadians) * cosp, 0.f);
  f.Normalize();
  return f;
}

void InputInitFromLookAt(float eyeX, float eyeY, float eyeZ, float targetX,
                         float targetY, float targetZ) {
  sCameraPosition = float4(eyeX, eyeY, eyeZ, 1.f);
  float4 f(targetX - eyeX, targetY - eyeY, targetZ - eyeZ, 0.f);
  f.Normalize();
  sYawRadians = std::atan2(f.x, f.z);
  sPitchRadians = std::asin(std::clamp(f.y, -1.f, 1.f));
  sFirstMouse = true;
}

#ifdef __ANDROID__

static constexpr float kTouchSensitivity = 0.004f;
static constexpr int kActionDown = 0;
static constexpr int kActionUp = 1;
static constexpr int kActionMove = 2;
static bool sTouchActive = false;
static int sTouchFingerCount = 0;

void InputOnTouchEvent(int action, float x, float y) {
  if (action == kActionDown) {
    sLastMouseX = x;
    sLastMouseY = y;
    sFirstMouse = false;
    sTouchActive = true;
    sTouchFingerCount = 1;
    return;
  }
  if (action == kActionUp) {
    sTouchActive = false;
    sTouchFingerCount = 0;
    return;
  }
  if (action == kActionMove && sTouchActive) {
    if (sFirstMouse) {
      sLastMouseX = x;
      sLastMouseY = y;
      sFirstMouse = false;
      return;
    }
    const double dx = x - sLastMouseX;
    const double dy = y - sLastMouseY;
    sLastMouseX = x;
    sLastMouseY = y;
    if (std::fabs(dx) + std::fabs(dy) < 0.5) {
      return;
    }
    sCameraMovedThisFrame = true;
    sYawRadians -= static_cast<float>(dx) * kTouchSensitivity;
    sPitchRadians -= static_cast<float>(dy) * kTouchSensitivity;
    sPitchRadians = std::clamp(sPitchRadians, -1.45f, 1.45f);
  }
}

void InputPollAndroid(float deltaTime) {
  if (sTouchActive && sTouchFingerCount >= 2) {
    float4 fwd = CameraForwardFromAngles();
    const float step = kMoveSpeed * deltaTime;
    sCameraPosition.x += fwd.x * step;
    sCameraPosition.y += fwd.y * step;
    sCameraPosition.z += fwd.z * step;
    sCameraMovedThisFrame = true;
  }
}

#else

void InputOnMouseMove(double xpos, double ypos) {
  if (!sMouseLookEnabled || !sWindow) {
    return;
  }
  if (sFirstMouse) {
    sLastMouseX = xpos;
    sLastMouseY = ypos;
    sFirstMouse = false;
    return;
  }
  const double dx = xpos - sLastMouseX;
  const double dy = ypos - sLastMouseY;
  sLastMouseX = xpos;
  sLastMouseY = ypos;
  if (std::fabs(dx) + std::fabs(dy) < 1e-8) {
    return;
  }
  sCameraMovedThisFrame = true;
  sYawRadians -= static_cast<float>(dx) * kMouseSensitivity;
  sPitchRadians -= static_cast<float>(dy) * kMouseSensitivity;
  sPitchRadians = std::clamp(sPitchRadians, -1.45f, 1.45f);
}

void InputAttach(GLFWwindow *window) {
  sWindow = window;
  glfwSetCursorPosCallback(
      window, [](GLFWwindow *, double x, double y) { InputOnMouseMove(x, y); });
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  sMouseLookEnabled = true;
  sFirstMouse = true;
}

void InputPoll(GLFWwindow *window, float deltaTime) {
  if (!window) {
    return;
  }

  float4 flatFwd(std::sin(sYawRadians), 0.f, std::cos(sYawRadians), 0.f);
  if (dot3(flatFwd, flatFwd) > 1e-8f) {
    flatFwd.Normalize();
  } else {
    flatFwd = float4(0.f, 0.f, 1.f, 0.f);
  }
  float4 flatRight(std::cos(sYawRadians), 0.f, -std::sin(sYawRadians), 0.f);
  flatRight.Normalize();

  float4 move(0.f, 0.f, 0.f, 0.f);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    move = move + flatFwd;
    sCameraMovedThisFrame = true;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    move = move - flatFwd;
    sCameraMovedThisFrame = true;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    move = move - flatRight;
    sCameraMovedThisFrame = true;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    move = move + flatRight;
    sCameraMovedThisFrame = true;
  }

  if (dot3(move, move) > 1e-12f) {
    move.Normalize();
    const float step = kMoveSpeed * deltaTime;
    sCameraPosition.x += move.x * step;
    sCameraPosition.y += move.y * step;
    sCameraPosition.z += move.z * step;
  }

  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    sCameraPosition.y += kMoveSpeed * deltaTime;
    sCameraMovedThisFrame = true;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    sCameraPosition.y -= kMoveSpeed * deltaTime;
    sCameraMovedThisFrame = true;
  }
}

void InputOnKeyDown(int keyCode) {
  if (keyCode == GLFW_KEY_ESCAPE && sWindow) {
    if (sMouseLookEnabled) {
      glfwSetInputMode(sWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      sMouseLookEnabled = false;
    } else {
      glfwSetInputMode(sWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      sMouseLookEnabled = true;
      sFirstMouse = true;
    }
  }
}

#endif

const float4 &InputCameraPosition() { return sCameraPosition; }

float4 InputCameraForwardUnit() { return CameraForwardFromAngles(); }

bool InputCameraMovedThisFrame() { return sCameraMovedThisFrame; }

void InputClearCameraMovedAfterFrame() { sCameraMovedThisFrame = false; }
