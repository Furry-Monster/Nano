#include "input/input.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

static GLFWwindow *sWindow = nullptr;
static float4 sCameraPosition(-330.f, 330.f, -330.f);
static float sYawRadians = 0.f;
static float sPitchRadians = 0.f;
static double sLastMouseX = 0.0;
static double sLastMouseY = 0.0;
static bool sFirstMouse = true;
static bool sMouseLookEnabled = true;
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

const float4 &InputCameraPosition() { return sCameraPosition; }

float4 InputCameraForwardUnit() { return CameraForwardFromAngles(); }

bool InputCameraMovedThisFrame() { return sCameraMovedThisFrame; }

void InputClearCameraMovedAfterFrame() { sCameraMovedThisFrame = false; }
