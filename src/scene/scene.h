#pragma once

#include <cstdint>
#include <string>

struct SceneStats {
  float fps{0.f};
  uint32_t lodMipValue{0};
  bool autoLod{true};
  float camX{0.f}, camY{0.f}, camZ{0.f};
};

void InitScene(int canvasWidth, int canvasHeight, const std::string &bvhPath,
               const std::string &meshPath);
void RenderOneFrame(float frameTime = 0.0f);
void OnKeyDown(int keyCode);
void OnKeyUp(int keyCode);

// Cross-platform LOD control (for Android UI buttons)
void SceneToggleAutoLOD();
void SceneLODUp();
void SceneLODDown();
SceneStats SceneGetStats();
