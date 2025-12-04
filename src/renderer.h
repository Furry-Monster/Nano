#pragma once

#include <memory>
#include "window.h"

namespace Nano
{
    class Window;
    class VulkanRenderer;

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

        // Forward methods to VulkanRenderer
        void beginFrame();
        void endFrame();

    private:
        std::shared_ptr<Window>         m_window;
        std::unique_ptr<VulkanRenderer> m_vulkan_renderer;
    };

} // namespace Nano