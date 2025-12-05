#include "application.h"

#include <chrono>
#include <memory>
#include "misc/logger.h"
#include "window/opengl/opengl_window.h"
#include "window/vulkan/vulkan_window.h"

namespace Nano
{
    Application::Application(const ApplicationConfig& config) : m_config(config), m_graphics_api(config.graphics_api)
    {
        m_logger = std::make_shared<Logger>(m_config.logger_config);

        switch (m_graphics_api)
        {
            case GraphicsAPI::Vulkan: {
                VulkanWindowConfig vulkan_config;
                vulkan_config.width     = m_config.window_config.width;
                vulkan_config.height    = m_config.window_config.height;
                vulkan_config.title     = m_config.window_config.title;
                vulkan_config.resizable = m_config.window_config.resizable;
                m_window                = std::make_shared<VulkanWindow>(vulkan_config);
                m_renderer              = std::make_shared<Renderer>(m_window);
                break;
            }
            case GraphicsAPI::OpenGL: {
                OpenGLWindowConfig opengl_config;
                opengl_config.width     = m_config.window_config.width;
                opengl_config.height    = m_config.window_config.height;
                opengl_config.title     = m_config.window_config.title;
                opengl_config.resizable = m_config.window_config.resizable;
                m_window                = std::make_shared<OpenGLWindow>(opengl_config);
                break;
            }
        }
    }

    Application::~Application() noexcept
    {
        if (m_renderer != nullptr)
            m_renderer.reset();

        if (m_window != nullptr)
            m_window.reset();

        if (m_logger != nullptr)
            m_logger.reset();
    }

    void Application::run()
    {
        m_start_time         = std::chrono::high_resolution_clock::now();
        m_previous_tick_time = m_start_time;

        while (!m_window->shouldClose())
        {
            auto delta_time = timeTick();
            logicalTick(delta_time);
            renderTick(delta_time);
        }
    }

    double Application::timeTick()
    {
        auto current_time = std::chrono::high_resolution_clock::now();

        auto delta_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(current_time - m_previous_tick_time);
        double delta_time = delta_duration.count() / 1000000.0;

        m_previous_tick_time = current_time;

        return delta_time;
    }

    void Application::logicalTick(double /*delta_time*/) { m_window->pollEvents(); }

    void Application::renderTick(double /*delta_time*/)
    {
        switch (m_graphics_api)
        {
            case GraphicsAPI::Vulkan: {
                if (m_renderer)
                {
                    m_renderer->beginFrame();
                    m_renderer->endFrame();
                }
                break;
            }
            case GraphicsAPI::OpenGL: {

                static_cast<OpenGLWindow*>(m_window.get())->swapBuffer();
                break;
            }
        }
    }
} // namespace Nano