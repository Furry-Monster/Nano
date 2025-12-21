#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <vulkan/vulkan_core.h>
#include <cstdint>

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

        bool     create(uint32_t width, uint32_t height);
        uint32_t acquireNextImage(VkSemaphore image_available_semaphore = VK_NULL_HANDLE);
        bool     present(uint32_t image_index, VkSemaphore render_finished_semaphore = VK_NULL_HANDLE);

        VkRenderPass   getRenderPass() const { return m_render_pass; }
        VkSwapchainKHR getSwapchain() const { return m_swapchain; }
        uint32_t       getImageCount() const { return m_image_cnt; }
        VkImage        getImage(uint32_t index) const;
        VkImageView    getImageView(uint32_t index) const;
        VkFramebuffer  getFramebuffer(uint32_t index) const;
        VkFormat       getFormat() const { return m_format; }
        VkExtent2D     getExtent() const { return m_extent; }

    private:
        bool initSwapchainProps(uint32_t width, uint32_t height, VkSwapchainCreateInfoKHR& swapchain_create_info);
        bool createImages();
        bool createRenderPass();
        bool createFramebuffers();
        void cleanup();

        VkSwapchainKHR m_swapchain {VK_NULL_HANDLE};
        uint32_t       m_image_cnt {0};
        VkImage*       m_images {nullptr};
        VkImageView*   m_image_views {nullptr};
        VkFramebuffer* m_framebuffers {nullptr};
        VkRenderPass   m_render_pass {VK_NULL_HANDLE};
        VkFormat       m_format {VK_FORMAT_UNDEFINED};
        VkExtent2D     m_extent {}; // width , height
    };

} // namespace Nano

#endif // !SWAPCHAIN_H
