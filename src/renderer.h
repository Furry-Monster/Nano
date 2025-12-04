#pragma once

namespace Nano
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() noexcept;

        Renderer(Renderer&&) noexcept            = default;
        Renderer& operator=(Renderer&&) noexcept = default;
        Renderer(const Renderer&)                = delete;
        Renderer& operator=(const Renderer&)     = delete;

        void init();
        void clean();

    private:
    };

} // namespace Nano