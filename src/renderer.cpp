#include "renderer.h"
#include "renderer/vulkan/vulkan_renderer.h"

namespace Nano
{
    Renderer::Renderer(std::shared_ptr<Window> window) : m_window(window)
    {
        m_vulkan_renderer = std::make_unique<VulkanRenderer>();
    }

    Renderer::~Renderer() noexcept { clean(); }

    void Renderer::init()
    {
        if (!m_window)
            return;

        m_vulkan_renderer->init(m_window);
    }

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