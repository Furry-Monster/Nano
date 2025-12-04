#pragma once

#include <memory>
#include "renderer.h"
#include "window.h"

namespace Nano
{
    class Application
    {
    public:
        Application();
        ~Application() noexcept;

        Application(Application&&) noexcept            = default;
        Application& operator=(Application&&) noexcept = default;
        Application(const Application&)                = delete;
        Application& operator=(const Application&)     = delete;

        void boot();

    private:
        void init();
        void run();
        void clean();

        std::shared_ptr<Window>   m_window {nullptr};
        std::shared_ptr<Renderer> m_renderer {nullptr};
    };
} // namespace Nano