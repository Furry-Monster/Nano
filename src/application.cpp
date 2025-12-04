#include "application.h"

#include <memory>

namespace Nano
{
    Application::~Application() noexcept { clean(); }

    void Application::init()
    {
        if (m_initialized)
            return;

        m_window = std::make_shared<Window>();
        m_window->init();

        m_renderer = std::make_shared<Renderer>(m_window);
        m_renderer->init();

        m_initialized = true;
    }

    void Application::run()
    {
        while (!m_window->shouldClose())
        {
            m_window->pollEvents();
        }
    }

    void Application::clean()
    {
        if (!m_initialized)
            return;

        m_renderer.reset();
        m_window.reset();

        m_initialized = false;
    }
} // namespace Nano