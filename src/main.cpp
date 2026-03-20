#include "render/vulkan_rhi.h"
#include "scene/scene.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <chrono>
#include <spdlog/spdlog.h>

static constexpr int kWindowWidth = 1280;
static constexpr int kWindowHeight = 720;

static void KeyCallback([[maybe_unused]] GLFWwindow *window, int key,
                        [[maybe_unused]] int scancode, int action,
                        [[maybe_unused]] int mods) {
  if (action == GLFW_RELEASE) {
    OnKeyUp(key);
  }
}

int main() {
  spdlog::set_level(spdlog::level::info);

  if (!glfwInit()) {
    spdlog::error("Failed to initialize GLFW");
    return -1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWwindow *window = glfwCreateWindow(
      kWindowWidth, kWindowHeight, "Nano - Virtual Geometry", nullptr, nullptr);
  if (window == nullptr) {
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

  InitScene(kWindowWidth, kWindowHeight);

  auto lastTime = std::chrono::high_resolution_clock::now();

  while (!static_cast<bool>(glfwWindowShouldClose(window))) {
    glfwPollEvents();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    RenderOneFrame(deltaTime);
  }

  vkDeviceWaitIdle(GetVulkanDevice());
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
