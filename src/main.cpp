#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <chrono>
#include "render/BattleFireVulkan.h"
#include "scene/Scene.h"

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE)
    {
        OnKeyUp(key);
    }
}

int main()
{
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int         canvasWidth  = 1280;
    int         canvasHeight = 720;
    GLFWwindow* window = glfwCreateWindow(canvasWidth, canvasHeight, "Nano Cluster Visualization", nullptr, nullptr);
    if (!window)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    printf("canvas : %d x %d\n", canvasWidth, canvasHeight);

    InitVulkanUserData initVulkanUserData = {window};
    bool               vulkanInited       = InitVulkan(&initVulkanUserData, canvasWidth, canvasHeight);
    if (!vulkanInited)
    {
        printf("ERROR: Failed to initialize Vulkan\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    InitScene(canvasWidth, canvasHeight);

    glfwSetKeyCallback(window, keyCallback);

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (vulkanInited)
        {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto frameTime   = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
            lastTime         = currentTime;
            float frameTimeInSecond = float(frameTime) / 1000.0f;

            RenderOneFrame(frameTimeInSecond);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
