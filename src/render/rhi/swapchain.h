#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

namespace Nano
{
    class Swapchain
    {
    public:
        Swapchain();
        ~Swapchain() noexcept;

        Swapchain(const Swapchain&)                = delete;
        Swapchain& operator=(const Swapchain&)     = delete;
        Swapchain(Swapchain&&) noexcept            = delete;
        Swapchain& operator=(Swapchain&&) noexcept = delete;

    private:
    };
} // namespace Nano

#endif // !SWAPCHAIN_H