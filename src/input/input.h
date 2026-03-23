#pragma once

#include "math/float4.h"

void InputInitFromLookAt(float eyeX, float eyeY, float eyeZ, float targetX,
                         float targetY, float targetZ);

#ifdef __ANDROID__
void InputOnTouchEvent(int action, int pointerCount, float x, float y);
void InputPollAndroid(float deltaTime);
#else
struct GLFWwindow;
void InputAttach(GLFWwindow *window);
void InputPoll(GLFWwindow *window, float deltaTime);
void InputOnMouseMove(double xpos, double ypos);
void InputOnKeyDown(int keyCode);
#endif

const float4 &InputCameraPosition();
float4 InputCameraForwardUnit();
bool InputCameraMovedThisFrame();
void InputClearCameraMovedAfterFrame();
