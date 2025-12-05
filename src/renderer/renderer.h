#pragma once

#include <memory>
#include "window/window.h"

namespace Nano
{
    class Window;

    class Renderer
    {
    public:
        explicit Renderer(std::shared_ptr<Window> window);
        virtual ~Renderer() noexcept = default;

        Renderer(Renderer&&) noexcept            = default;
        Renderer& operator=(Renderer&&) noexcept = default;
        Renderer(const Renderer&)                = delete;
        Renderer& operator=(const Renderer&)     = delete;

        virtual void beginFrame() = 0;
        virtual void endFrame()   = 0;

    protected:
        std::shared_ptr<Window> m_window;
    };

} // namespace Nano
