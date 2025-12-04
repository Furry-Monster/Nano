#include "application.h"

#include <memory>

namespace Nano
{
    Application::Application()
    {
        m_window   = std::make_shared<Window>();
        m_renderer = std::make_shared<Renderer>();
    }

    Application::~Application() noexcept { clean(); }

    void Application::boot()
    {
        init();
        run();
    }

    void Application::init()
    {
        m_window->init();
        m_renderer->init();
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
        m_renderer.reset();
        m_window.reset();
    }
} // namespace Nano