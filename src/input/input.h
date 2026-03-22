#pragma once

#include "math/float4.h"

struct GLFWwindow;

void InputInitFromLookAt(float eyeX, float eyeY, float eyeZ, float targetX,
                         float targetY, float targetZ);
void InputAttach(GLFWwindow *window);
void InputPoll(GLFWwindow *window, float deltaTime);
void InputOnMouseMove(double xpos, double ypos);
void InputOnKeyDown(int keyCode);

const float4 &InputCameraPosition();
float4 InputCameraForwardUnit();
bool InputCameraMovedThisFrame();
void InputClearCameraMovedAfterFrame();
