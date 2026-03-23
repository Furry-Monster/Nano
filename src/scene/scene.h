#pragma once

#include <string>

void InitScene(int canvasWidth, int canvasHeight, const std::string &bvhPath,
               const std::string &meshPath);
void RenderOneFrame(float frameTime = 0.0f);
void OnKeyDown(int keyCode);
void OnKeyUp(int keyCode);
