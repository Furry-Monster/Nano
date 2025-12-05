#include "renderer.h"
#include "renderer/vulkan/vulkan_renderer.h"

namespace Nano
{
    Renderer::Renderer(std::shared_ptr<Window> window) : m_window(window)
    {
        if (m_window)
        {
            m_vulkan_renderer = std::make_unique<VulkanRenderer>();
            m_vulkan_renderer->init(m_window);
        }
    }

    Renderer::~Renderer() noexcept { clean(); }

    void Renderer::clean()
    {
        if (m_vulkan_renderer)
        {
            m_vulkan_renderer->clean();
        }
    }

    void Renderer::beginFrame()
    {
        if (m_vulkan_renderer)
        {
            m_vulkan_renderer->beginFrame();
        }
    }

    void Renderer::endFrame()
    {
        if (m_vulkan_renderer)
        {
            m_vulkan_renderer->endFrame();
        }
    }

} // namespace Nano