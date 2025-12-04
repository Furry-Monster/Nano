#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>
#include "vulkan_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_swapchain.h"

namespace Nano
{
    class Window;

    class VulkanRenderer
    {
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() noexcept;

        VulkanRenderer(VulkanRenderer&&) noexcept;
        VulkanRenderer& operator=(VulkanRenderer&&) noexcept;
        VulkanRenderer(const VulkanRenderer&)            = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        void init(std::shared_ptr<Window> window);
        void clean();

        void beginFrame();
        void endFrame();
        bool isFrameInFlight() const { return m_frame_in_flight; }

        VkDevice               getDevice() const { return m_device.getDevice(); }
        VkPhysicalDevice       getPhysicalDevice() const { return m_device.getPhysicalDevice(); }
        VkQueue                getGraphicsQueue() const { return m_device.getGraphicsQueue(); }
        VkQueue                getPresentQueue() const { return m_device.getPresentQueue(); }
        VkCommandPool          getCommandPool() const { return m_command_pool.getCommandPool(); }
        VkSurfaceKHR           getSurface() const { return m_surface; }
        const VulkanSwapchain& getSwapchain() const { return m_swapchain; }
        uint32_t               getCurrentFrameIndex() const { return m_current_frame; }

        void waitIdle() const { m_device.waitIdle(); }

    private:
        void createSyncObjects();
        void cleanupSyncObjects();
        void recreateSwapchain();

        std::shared_ptr<Window> m_window;
        VulkanInstance          m_instance;
        VulkanDevice            m_device;
        VulkanSwapchain         m_swapchain;
        VulkanCommandPool       m_command_pool;
        VkSurfaceKHR            m_surface {VK_NULL_HANDLE};

        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence>     m_in_flight_fences;
        std::vector<VkFence>     m_images_in_flight;

        uint32_t m_current_frame {0};
        bool     m_frame_in_flight {false};
        bool     m_framebuffer_resized {false};
    };

} // namespace Nano
