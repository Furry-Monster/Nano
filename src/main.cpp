#include "input/input.h"
#include "render/vulkan_rhi.h"
#include "scene/scene.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <string>

static constexpr int kWindowWidth = 1280;
static constexpr int kWindowHeight = 720;
static const char *kDefaultBvh = "res/Mitsuba/mitsuba.bvh";
static const char *kDefaultMesh = "res/Mitsuba/mitsuba.nanomesh";

static void PrintUsage(const char *exe) {
  std::cout << "Usage: " << exe << " [options]\n"
            << "  --bvh <path>   BVH file (default: " << kDefaultBvh << ")\n"
            << "  --mesh <path>  NanoMesh file (default: " << kDefaultMesh
            << ")\n"
            << "  -h, --help     Show this help\n";
}

static bool ParseArgs(int argc, char **argv, std::string &bvhPath,
                      std::string &meshPath) {
  bvhPath = kDefaultBvh;
  meshPath = kDefaultMesh;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return false;
    }
    if ((arg == "--bvh" || arg == "--mesh") && i + 1 < argc) {
      if (arg == "--bvh")
        bvhPath = argv[++i];
      else
        meshPath = argv[++i];
    }
  }
  return true;
}

static void KeyCallback([[maybe_unused]] GLFWwindow *window, int key,
                        [[maybe_unused]] int scancode, int action,
                        [[maybe_unused]] int mods) {
  if (action == GLFW_PRESS)
    OnKeyDown(key);
  else if (action == GLFW_RELEASE)
    OnKeyUp(key);
}

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::info);

  std::string bvhPath, meshPath;
  if (!ParseArgs(argc, argv, bvhPath, meshPath))
    return 0;

  if (!glfwInit()) {
    spdlog::error("Failed to initialize GLFW");
    return -1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(kWindowWidth, kWindowHeight, "Nano", nullptr, nullptr);
  if (!window) {
    spdlog::error("Failed to create GLFW window");
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, KeyCallback);

  if (!InitVulkan(window, kWindowWidth, kWindowHeight)) {
    spdlog::error("Failed to initialize Vulkan");
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  try {
    InitScene(kWindowWidth, kWindowHeight, bvhPath, meshPath);
  } catch (const std::exception &e) {
    spdlog::error("{}", e.what());
    glfwDestroyWindow(window);
    glfwTerminate();
    return -1;
  }

  InputAttach(window);

  auto lastTime = std::chrono::high_resolution_clock::now();
  auto lastTitleUpdate = lastTime;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    InputPoll(window, deltaTime);
    RenderOneFrame(deltaTime);

    auto elapsed =
        std::chrono::duration<float>(currentTime - lastTitleUpdate).count();
    if (elapsed >= 0.25f) {
      lastTitleUpdate = currentTime;
      SceneStats stats = SceneGetStats();
      std::ostringstream oss;
      oss << "Nano | FPS: " << static_cast<int>(stats.fps + 0.5f)
          << " | LOD: " << (stats.autoLod ? "auto" : "manual") << " ("
          << stats.lodMipValue << ") | Cam: " << static_cast<int>(stats.camX)
          << "," << static_cast<int>(stats.camY) << ","
          << static_cast<int>(stats.camZ);
      glfwSetWindowTitle(window, oss.str().c_str());
    }
  }

  vkDeviceWaitIdle(GetVulkanDevice());
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
