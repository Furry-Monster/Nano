#include "renderer.h"
#include <vulkan/vulkan_core.h>
#include "window.h"

namespace Nano
{
    Renderer::Renderer(std::shared_ptr<Window> window) : m_window(window) {}

    Renderer::~Renderer() noexcept { clean(); }

    void Renderer::init()
    {
        if (!m_window)
            return;

        createInstance();
        setupDebugger();
        createSurface();
        selectPhysicalDevice();

        createLogicalDevice();
        createSwapChain();
        createImageViews();

        createGraphicPipeline();

        createCommandPool();

        createCommandBuffers();
        createSyncObjects();
    }

    void Renderer::clean() {}

    void Renderer::createInstance()
    {
        VkApplicationInfo app_info {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = m_window->getTitle();
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName        = "Nano Engine\0";
        app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion         = VK_API_VERSION_1_4;

        VkInstanceCreateInfo instance_create_info {};
        instance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &app_info;

        
    }
    void Renderer::setupDebugger() {}
    void Renderer::createSurface() {}
    void Renderer::selectPhysicalDevice() {}
    void Renderer::createLogicalDevice() {}
    void Renderer::createSwapChain() {}
    void Renderer::createImageViews() {}
    void Renderer::createGraphicPipeline() {}
    void Renderer::createCommandPool() {}
    void Renderer::createCommandBuffers() {}
    void Renderer::createSyncObjects() {}

} // namespace Nano