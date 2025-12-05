#include "application.h"

#include <chrono>
#include <memory>
#include "misc/logger.h"
#include "renderer/opengl/opengl_renderer.h"
#include "renderer/vulkan/vulkan_renderer.h"
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
                m_window   = std::make_shared<VulkanWindow>(config.window_config);
                m_renderer = std::make_shared<VulkanRenderer>(m_window);
                break;
            }
            case GraphicsAPI::OpenGL: {
                m_window   = std::make_shared<OpenGLWindow>(config.window_config);
                m_renderer = std::make_shared<OpenGLRenderer>(m_window);
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

    void Application::logicalTick(double /*delta_time*/)
    {
        m_window->pollEvents();
        switch (m_graphics_api)
        {
            case GraphicsAPI::Vulkan:
                break;
            case GraphicsAPI::OpenGL:

                break;
        }
    }

    void Application::renderTick(double /*delta_time*/)
    {
        switch (m_graphics_api)
        {
            case GraphicsAPI::Vulkan: {
                m_renderer->beginFrame();
                m_renderer->endFrame();
                break;
            }
            case GraphicsAPI::OpenGL: {
                m_renderer->beginFrame();
                m_renderer->endFrame();
                break;
            }
        }
    }
} // namespace Nano