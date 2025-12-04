#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include "window.h"

namespace Nano
{
    class Window;

    class Renderer
    {
    public:
        explicit Renderer(std::shared_ptr<Window> window);
        ~Renderer() noexcept;

        Renderer(Renderer&&) noexcept            = default;
        Renderer& operator=(Renderer&&) noexcept = default;
        Renderer(const Renderer&)                = delete;
        Renderer& operator=(const Renderer&)     = delete;

        void init();
        void clean();

    private:
        std::shared_ptr<Window> m_window;
        bool                    m_enable_validation_layers = true;

        VkInstance       m_instance;
        VkSurfaceKHR     m_surface;
        VkPhysicalDevice m_physical_device;
        VkDevice         m_device;
        VkRenderPass     m_render_pass;

        void createInstance();
        void setupDebugger();
        void createSurface();
        void selectPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createGraphicPipeline();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
    };

} // namespace Nano